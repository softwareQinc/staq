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

#include <tweedledee/qasm/ast/ast.hpp>

namespace synthewareQ {

  namespace qasm = tweedledee::qasm;

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

    tweedledum::gg_network<tweedledum::io3_gate> q_net;
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
    caterpillar::logic_network_synthesis(q_net, *lutn, strategy, tweedledum::stg_from_spectrum(), p, &stats);

    // Decompose Toffolis in terms of Clifford + T
    tweedledum::dt_decomposition(q_net);


    /* QASM building */

    // Allocate ancillas
    uint32_t num_qubits = q_net.num_qubits();
    uint32_t num_inputs = stats.i_indexes.size() + stats.o_indexes.size();

    if (ctx_->find_declaration("anc_")) {
      std::cerr << "WARNING: local register anc_ shadows previous declaration" << std::endl;
    }
    auto anc_decl = qasm::decl_ancilla::build(ctx_, location, "anc_", num_qubits - num_inputs, false);
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
        id_refs[j] = [&]() {
          auto indexed_reference = qasm::expr_reg_idx_ref::builder(ctx_, location);
          indexed_reference.add_child(qasm::expr_decl_ref::build(ctx_, location, anc_decl));
          indexed_reference.add_child(qasm::expr_integer::create(ctx_, location, i++));
          return indexed_reference.finish();
        };
      }
    }
    
    //
    q_net.foreach_gate([&](auto const& node) {
        auto const& gate = node.gate;
        switch (gate.operation()) {
        case tweedledum::gate_set::u3:
          std::cerr << "WARNING: u3 gate not currently supported" << std::endl;
          break;
          
        case tweedledum::gate_set::hadamard:
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
        case tweedledum::gate_set::rotation_x:
        case tweedledum::gate_set::rotation_y:
        case tweedledum::gate_set::rotation_z:

        case tweedledum::gate_set::pauli_x:
        case tweedledum::gate_set::pauli_y:

        case tweedledum::gate_set::t:
        case tweedledum::gate_set::phase:
        case tweedledum::gate_set::pauli_z:
        case tweedledum::gate_set::phase_dagger:
        case tweedledum::gate_set::t_dagger:
          break;

        case tweedledum::gate_set::cx:
          {
            auto stmt_builder = qasm::stmt_cnot::builder(ctx_, location);

            stmt_builder.add_child(id_refs[gate.control()]());
            stmt_builder.add_child(id_refs[gate.target()]());

            builder.add_child(stmt_builder.finish());
          }
          break;
        case tweedledum::gate_set::cz:
        case tweedledum::gate_set::swap:

        case tweedledum::gate_set::mcx:
        case tweedledum::gate_set::mcz:

        default:
          std::cerr << "Error: lhrs returned unsupported gate type" << std::endl;
          break;

        }
      });

    return builder.finish();
  }
    

}
