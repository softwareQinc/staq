/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "transformations/oracle_synthesizer.hpp"
#include "qasm/visitors/source_printer.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <iostream>

using namespace synthewareQ;

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }
  auto program = qasm::read_from_file(argv[1]);
  if (program) {
    transformations::expand_oracles(program.get());

    qasm::print_source(program.get());
  }
}
