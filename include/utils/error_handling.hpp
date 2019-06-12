/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#include <cstdlib>
#include <iostream>
#include <fmt/format.h>

namespace synthewareQ {

void report_bad_alloc_error()
{
	std::cerr << "synthewareQ ERROR: out of memory (OOM)\n";
	std::abort();
}

void unreachable_impl(char const* msg, char const* file, unsigned line)
{
	if (msg) {
		std::cerr << msg << "\n";
	}
	std::cerr << "UNREACHABLE executed";
	if (file) {
		std::cerr << " at " << file << ":" << line;
	}
	std::cerr << "!\n";
	std::abort();
}

} // namespace synthewareQ
