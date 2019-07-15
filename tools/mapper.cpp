/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "qasm/visitors/source_printer.hpp"

#include "transformations/inline.hpp"

#include "mapping/device.hpp"
#include "mapping/layout/basic.hpp"
#include "mapping/mapping/swap.hpp"

using namespace synthewareQ;

// TODO: Find or create a format for reading machine definitions
// and have this tool accept a machine definition as input for mapping

int main() {

  auto program = qasm::read_from_stdin();
  if (program) {
    // Inline fully first
    auto preprocessor = transformations::inliner(program.get(), { false, {}, "anc" });
    preprocessor.visit(*program);
    // Initial layout
    auto physical_layout = mapping::compute_layout(program.get(), mapping::rigetti_8q);
    mapping::apply_layout(program.get(), physical_layout);
    // Mapping
    mapping::map_onto_device(program.get(), mapping::rigetti_8q);
    qasm::print_source(program.get());
  } else {
    std::cerr << "Parsing failed\n";
  }

  return 1;
}
