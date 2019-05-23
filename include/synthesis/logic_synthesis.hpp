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

  /*! \brief LUT-based hierarchical logic synthesis (arXiv:1706.02721) of classical logic
   *  files, based on the example given in caterpillar */

  template<typename T>
  tweedledum::gg_network<tweedledum::io3_gate> lhrs(T network) {

    // Map network into lut with "cut size" 4
    mockturtle::mapping_view<T, true> mapped_network{network};

    mockturtle::lut_mapping_params ps;
    ps.cut_enumeration_ps.cut_size = 3;
    mockturtle::lut_mapping< mockturtle::mapping_view<T, true>, true>(mapped_network, ps);

    // Collapse network into a klut network
    auto lutn = mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_network);

    tweedledum::gg_network<tweedledum::io3_gate> rev_net;
    if (!lutn) {
      std::cerr << "Could not map network into klut network" << std::endl;
      return rev_net;
    }

    // Synthesize a gate graph network with 1, 2, and 3 qubit gates using
    // hierarchical synthesis and spectral analysis for klut synthesis.
    // Mapping strategy is eager.

    auto strategy = caterpillar::eager_mapping_strategy<mockturtle::klut_network>();
    caterpillar::logic_network_synthesis(rev_net, *lutn, strategy, tweedledum::stg_from_spectrum());

    // Decompose Toffolis in terms of Clifford + T
    tweedledum::dt_decomposition(rev_net);

    return rev_net;

  }


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

  template<typename T>
  tweedledee::qasm::ast_node* to_qasm(tweedledee::qasm::ast_context* ctx_, uint32_t location, T& q_net) {
    auto builder = tweedledee::qasm::list_gops::builder(ctx_, location);
    return builder.finish();
  }
    

}
