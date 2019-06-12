/*-------------------------------------------------------------------------------------------------
  | This file is distributed under the MIT License.
  | See accompanying file /LICENSE for details.
  | Author(s): Matthew Amy
  *------------------------------------------------------------------------------------------------*/

#include <lorina/lorina.hpp>
#include <mockturtle/mockturtle.hpp>
#include <tweedledum/tweedledum.hpp>
#include <caterpillar/synthesis/lhrs.hpp>
#include <caterpillar/synthesis/strategies/bennett_mapping_strategy.hpp>
#include <caterpillar/synthesis/strategies/eager_mapping_strategy.hpp>
#include <unordered_map>

#include "qasm/ast/ast.hpp"

namespace synthewareQ {

  namespace angles = tweedledum::angles;

  enum class format { 
    binary_aiger, 
      ascii_aiger, 
      bench, 
      blif, 
      pla, 
      verilog,
      };

  std::unordered_map<std::string, format> ext_to_format( { 
      {"aig", format::binary_aiger}, 
      {"aag", format::ascii_aiger}, 
      {"bench", format::bench}, 
      {"blif", format::blif}, 
      {"pla", format::pla}, 
      {"v", format::verilog}, 
  } );

  mockturtle::mig_network read_from_file(std::string fname) {

    const auto i = fname.find_last_of(".");
    if (i == std::string::npos) {
      std::cerr << "No filename extension" << std::endl;
    }
 
    const auto it = ext_to_format.find(fname.substr(i+1));
    if (it == ext_to_format.end()) {
      std::cerr << "Unrecognized file format" << std::endl;
    }

    // Read input file into a logic network
    mockturtle::mig_network mig;
    switch(it->second) {
    case format::binary_aiger:
      lorina::read_aiger(fname, mockturtle::aiger_reader(mig));
      break;
    case format::ascii_aiger:
      lorina::read_ascii_aiger(fname, mockturtle::aiger_reader(mig));
      break;
    case format::verilog:
      lorina::read_verilog(fname, mockturtle::verilog_reader(mig));
      break;
    default:
      std::cerr << "File type not currently cupported" << std::endl;
    }

    return mig;
  }

  /*! \brief Returns a qasm expr node with the value of the given angle */
  qasm::ast_node* angle_to_expr(qasm::ast_context* ctx_, uint32_t location, tweedledum::angle theta) {
    auto sval = theta.symbolic_value();
    if (sval == std::nullopt) {
      // Angle is real-valued
      return qasm::expr_real::create(ctx_, location, theta.numeric_value());
    } else {
      // Angle is of the form pi*(a/b) for a & b integers
      // NOTE: tweedledum::gate base and tweedledum::angle contradict -- the former defines t as
      //       an angle of 1/4, while the latter gives it as an angle of 1/8.
      auto [a, b] = sval.value();

      if (a == 0) {
        return qasm::expr_integer::create(ctx_, location, 0);
      } else if (a == 1) {
        auto total = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
        total.add_child(qasm::expr_pi::create(ctx_, location));
        total.add_child(qasm::expr_integer::create(ctx_, location, b));

        return total.finish();
      } else if (a == -1) {
        auto numerator = qasm::expr_unary_op::builder(ctx_, location, qasm::unary_ops::minus);
        numerator.add_child(qasm::expr_pi::create(ctx_, location));

        auto total = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
        total.add_child(numerator.finish());
        total.add_child(qasm::expr_integer::create(ctx_, location, b));

        return total.finish();
      } else {
        auto numerator = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::multiplication);
        numerator.add_child(qasm::expr_integer::create(ctx_, location, a));
        numerator.add_child(qasm::expr_pi::create(ctx_, location));

        auto total = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
        total.add_child(numerator.finish());
        total.add_child(qasm::expr_integer::create(ctx_, location, b));

        return total.finish();
      }
    }
  }

  /*! \brief LUT-based hierarchical logic synthesis (arXiv:1706.02721) of classical logic
   *  files, based on the example given in caterpillar */

  template<typename T>
  qasm::ast_node* synthesize(qasm::ast_context* ctx_, uint32_t location, T& l_net, qasm::list_ids* params) {
    auto builder = qasm::list_gops::builder(ctx_, location);

    // Map network into lut with "cut size" 4
    mockturtle::mapping_view<T, true> mapped_network{l_net};

    mockturtle::lut_mapping_params ps;
    ps.cut_enumeration_ps.cut_size = 3;
    mockturtle::lut_mapping< mockturtle::mapping_view<T, true>, true>(mapped_network, ps);

    // Collapse network into a klut network
    auto lutn = mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_network);

    tweedledum::gg_network<tweedledum::mcmt_gate> q_net;
    if (!lutn) {
      std::cerr << "Could not map network into klut network" << std::endl;
      return builder.finish();
    }

    // Synthesize a gate graph network with 1, 2, and 3 qubit gates using
    // hierarchical synthesis and spectral analysis for klut synthesis.
    // Mapping strategy is eager.

    auto strategy = caterpillar::eager_mapping_strategy<mockturtle::klut_network>();
    caterpillar::logic_network_synthesis_params p;
    caterpillar::logic_network_synthesis_stats stats;
    caterpillar::logic_network_synthesis(q_net, *lutn, strategy, tweedledum::stg_from_pkrm(), p, &stats);

    // Decompose Toffolis in terms of at most 3-control Toffolis
    q_net = tweedledum::barenco_decomposition(q_net, { 3 });
    // Decompose further into Clifford + T
    q_net = tweedledum::dt_decomposition(q_net);
    // Optimize
    q_net = tweedledum::phase_folding(q_net);


    /* QASM building */

    // Allocate ancillas
    uint32_t num_qubits = q_net.num_qubits();
    uint32_t num_inputs = stats.i_indexes.size() + stats.o_indexes.size();

    if (ctx_->find_declaration("anc")) {
      std::cerr << "WARNING: local register anc_ shadows previous declaration" << std::endl;
    }
    auto anc_decl = qasm::decl_ancilla::build(ctx_, location, "anc", num_qubits - num_inputs, false);
    builder.add_child(anc_decl);

    // Create a mapping from qubits to functions generating declaration references
    auto inputs = stats.i_indexes;
    inputs.insert(inputs.end(), stats.o_indexes.begin(), stats.o_indexes.end());
    assert(params->num_children() == inputs.size()); //sanity check, should be checked in semantic analysis phase

    auto i = 0;
    std::vector<std::function<qasm::ast_node*()> > id_refs(num_qubits);
    for (auto& param : *params) {
      id_refs[inputs[i++]] = [&]() { return qasm::expr_decl_ref::build(ctx_, location, &param); };
    }

    i = 0;
    for (auto j = 0; j < num_qubits; j++) {
      if (!id_refs[j]) {
        id_refs[j] = [&, i]() {
          auto indexed_reference = qasm::expr_reg_idx_ref::builder(ctx_, location);
          indexed_reference.add_child(qasm::expr_decl_ref::build(ctx_, location, anc_decl));
          indexed_reference.add_child(qasm::expr_integer::create(ctx_, location, i));
          return indexed_reference.finish();
        };
        i++;
      }
    }
    
    q_net.foreach_gate([&](auto const& node) {
        auto const& gate = node.gate;
        switch (gate.operation()) {
        case tweedledum::gate_lib::u3:
          std::cerr << "WARNING: u3 gate not currently supported" << std::endl;
          break;
          
        case tweedledum::gate_lib::hadamard:
          {
            // If there exists a declaration for the Hadamard gate, use that
            auto declaration = ctx_->find_declaration("h");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              theta.add_child(qasm::expr_pi::create(ctx_, location));
              theta.add_child(qasm::expr_integer::create(ctx_, location, 2));
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_pi::create(ctx_, location);

              stmt_builder.add_child(theta.finish());
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::rotation_x:
          std::cerr << "WARNING: X-rotation gate not currently supported" << std::endl;
          break;
        case tweedledum::gate_lib::rotation_y:
          std::cerr << "WARNING: Y-rotation gate not currently supported" << std::endl;
          break;
        case tweedledum::gate_lib::rotation_z:
          {
            auto expr_angle = angle_to_expr(ctx_, location, gate.rotation_angle());
            auto declaration = ctx_->find_declaration("rz");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(expr_angle);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_integer::create(ctx_, location, 0);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(expr_angle);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;

        case tweedledum::gate_lib::pauli_x:
          {
            auto declaration = ctx_->find_declaration("x");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_pi::create(ctx_, location);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_pi::create(ctx_, location);

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::pauli_y:
          {
            auto declaration = ctx_->find_declaration("y");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_pi::create(ctx_, location);
              auto phi = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              phi.add_child(qasm::expr_pi::create(ctx_, location));
              phi.add_child(qasm::expr_integer::create(ctx_, location, 2));
              auto lambda = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              lambda.add_child(qasm::expr_pi::create(ctx_, location));
              lambda.add_child(qasm::expr_integer::create(ctx_, location, 2));

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi.finish());
              stmt_builder.add_child(lambda.finish());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;

        case tweedledum::gate_lib::t:
          {
            auto declaration = ctx_->find_declaration("t");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_integer::create(ctx_, location, 0);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              lambda.add_child(qasm::expr_pi::create(ctx_, location));
              lambda.add_child(qasm::expr_integer::create(ctx_, location, 4));

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda.finish());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::phase:
          {
            auto declaration = ctx_->find_declaration("s");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_integer::create(ctx_, location, 0);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              lambda.add_child(qasm::expr_pi::create(ctx_, location));
              lambda.add_child(qasm::expr_integer::create(ctx_, location, 2));

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda.finish());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::pauli_z:
          {
            auto declaration = ctx_->find_declaration("z");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_integer::create(ctx_, location, 0);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_pi::create(ctx_, location);

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::phase_dagger:
          {
            auto declaration = ctx_->find_declaration("sdg");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_integer::create(ctx_, location, 0);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_unary_op::builder(ctx_, location, qasm::unary_ops::minus);
              auto tmp = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              tmp.add_child(qasm::expr_pi::create(ctx_, location));
              tmp.add_child(qasm::expr_integer::create(ctx_, location, 2));
              lambda.add_child(tmp.finish());

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda.finish());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::t_dagger:
          {
            auto declaration = ctx_->find_declaration("tdg");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_unitary::builder(ctx_, location);

              auto theta = qasm::expr_integer::create(ctx_, location, 0);
              auto phi = qasm::expr_integer::create(ctx_, location, 0);
              auto lambda = qasm::expr_unary_op::builder(ctx_, location, qasm::unary_ops::minus);
              auto tmp = qasm::expr_binary_op::builder(ctx_, location, qasm::binary_ops::division);
              tmp.add_child(qasm::expr_pi::create(ctx_, location));
              tmp.add_child(qasm::expr_integer::create(ctx_, location, 4));
              lambda.add_child(tmp.finish());

              stmt_builder.add_child(theta);
              stmt_builder.add_child(phi);
              stmt_builder.add_child(lambda.finish());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;

        case tweedledum::gate_lib::cx:
          {
            // Use the qelib declaration for informity
            auto declaration = ctx_->find_declaration("cx");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.control()]());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              auto stmt_builder = qasm::stmt_cnot::builder(ctx_, location);

              stmt_builder.add_child(id_refs[gate.control()]());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            }
          }
          break;
        case tweedledum::gate_lib::cz:
          {
            auto declaration = ctx_->find_declaration("cz");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              stmt_builder.add_child(id_refs[gate.control()]());
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              std::cerr << "Error: cz requires previous definition" << std::endl;
            }
          }
          break;
        case tweedledum::gate_lib::mcx:
          // Must have at most 2 controls (i.e. Toffoli gate)
          {
            if (gate.num_controls() > 2) {
              std::cerr << "Error: too many controls (" << gate.num_controls() << ")" << std::endl;
              break;
            }
            auto declaration = ctx_->find_declaration("ccx");
            if (declaration) {
              auto stmt_builder = qasm::stmt_gate::builder(ctx_, location);

              auto decl_ref = qasm::expr_decl_ref::build(ctx_, location, declaration);
              stmt_builder.add_child(decl_ref);
              gate.foreach_control([&](auto const& qubit) {
                  stmt_builder.add_child(id_refs[qubit]());
              });
              stmt_builder.add_child(id_refs[gate.target()]());

              builder.add_child(stmt_builder.finish());
            } else {
              std::cerr << "Error: ccx requires previous definition" << std::endl;
            }
          }
          break;

        case tweedledum::gate_lib::mcz:
        case tweedledum::gate_lib::swap:
        default:
          std::cerr << "Error: unsupported gate" << std::endl;
          break;

        }
      });

    return builder.finish();
  }
    

}
