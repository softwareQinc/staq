/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "optimization/rotation_folding.hpp"

using namespace synthewareQ;

int main() {

  auto program = qasm::read_from_stdin();
  if (program) {
    rotation_fold(*program);
    qasm::print_source(program.get());
  } else {
    std::cerr << "Parsing failed\n";
  }

  return 1;
}
