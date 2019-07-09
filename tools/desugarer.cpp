/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "transformations/desugarer.hpp"

using namespace synthewareQ;

int main(int argc, char** argv) {

  auto program = qasm::read_from_stdin();
  if (program) {
    transformations::desugar(program.get());
    qasm::print_source(program.get());
  } else {
    std::cerr << "Parsing failed\n";
  }

  return 1;
}
