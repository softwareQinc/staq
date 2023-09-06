/*
 * Goal:
 * 1. Parse qasm from stdin
 * 2. Detect rz instructions
 * 3. Replace them
 */

#include "transformations/synth.hpp"
#include "qasmtools/parser/parser.hpp"

#include "grid_synth/gmp_functions.hpp"

int main() {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    using namespace std;

    auto program = parse_stdin("", true);
    if (program) {
        transformations::replace_rz(*program);
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
