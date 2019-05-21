/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY = true

#include "transformations/logic_elaborator.hpp"

#include <fmt/color.h>
#include <fmt/format.h>
#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Input file not specified.\n";
	}
	auto program = tweedledee::qasm::read_from_file(argv[1]);
	if (program) {
      synthewareQ::logic_elaborator elaborator(0);
	  elaborator.visit(*program);
	}
}
