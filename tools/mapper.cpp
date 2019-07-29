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
#include "mapping/layout/eager.hpp"
#include "mapping/layout/bestfit.hpp"
#include "mapping/mapping/swap.hpp"
#include "mapping/mapping/steiner.hpp"

#include <CLI/CLI.hpp>

using namespace synthewareQ;

// TODO: Find or create a format for reading machine definitions
// and have this tool accept a machine definition as input for mapping

int main(int argc, char** argv) {
  std::string layout = "linear";
  std::string mapper = "swap";

  CLI::App app{ "Physical mapper" };

  app.add_option("-l", layout, "Layout algorithm to use (linear|eager|bestfit)");
  app.add_option("-m", mapper, "Mapping algorithm to use (swap|steiner)");

  CLI11_PARSE(app, argc, argv);

  auto program = qasm::read_from_stdin();
  if (program) {
    // Inline fully first
    auto preprocessor = transformations::inliner(program.get(), { false, {}, "anc" });
    preprocessor.visit(*program);

    // Initial layout
    mapping::layout physical_layout;
    if (layout == "linear") {
      physical_layout = mapping::compute_layout(program.get(), mapping::rigetti_8q);
    } else if (layout == "eager") {
      physical_layout = mapping::compute_layout_eager(program.get(), mapping::rigetti_8q);
    } else if (layout == "bestfit") {
      physical_layout = mapping::compute_layout_bestfit(program.get(), mapping::rigetti_8q);
    }
    mapping::apply_layout(program.get(), physical_layout);

    // Mapping
    if (mapper == "swap") {
      mapping::map_onto_device(program.get(), mapping::rigetti_8q);
    } else if (mapper == "steiner") {
      mapping::steiner_mapping(program.get(), mapping::rigetti_8q);
    }

    // Print result
    qasm::print_source(program.get());
  } else {
    std::cerr << "Parsing failed\n";
  }

  return 1;
}
