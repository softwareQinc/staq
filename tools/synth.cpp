/*
 * Goal:
 * 1. Parse qasm from stdin
 * 2. Detect rz instructions
 * 3. Replace them
 */

#include "transformations/synth.hpp"
#include "qasmtools/parser/parser.hpp"

#include "grid_synth/gmp_functions.hpp"

#include <gmpxx.h>

int main() {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    // we need to do this BEFORE we parse so that floats are parsed with enough prec
    int prec = 17;
    grid_synth::DEFAULT_GMP_PREC = 4 * prec + 19;
    mpf_set_default_prec(log(10) / log(2) * grid_synth::DEFAULT_GMP_PREC);

    auto program = parse_stdin("", true);
    if (program) {
        transformations::replace_rz(*program);
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
