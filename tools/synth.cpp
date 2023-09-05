/*
 * Goal:
 * 1. Parse qasm from stdin
 * 2. Detect rz instructions
 * 3. Replace them
 */

#include "qasmtools/parser/parser.hpp"
#include "transformations/synth.hpp"

int main() {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    auto program = parse_stdin();
    if (program) {
        transformations::desugar(*program);
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
