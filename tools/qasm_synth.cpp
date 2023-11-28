/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2023 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>

#include <libs/CLI/CLI.hpp>

#include "grid_synth/gmp_functions.hpp"
#include "grid_synth/types.hpp"
#include "qasmtools/parser/parser.hpp"
#include "transformations/qasm_synth.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;
    using qasmtools::parser::parse_stdin;

    bool check = false, details = false, verbose = false;
    long int prec;
    int factor_effort;
    domega_matrix_table_t s3_table;

    CLI::App app{"Grid Synthesis rx/ry/rz substitution in OpenQASM 2.0 files"};

    CLI::Option* prec_opt =
        app.add_option<long int, int>(
               "-p, --precision", prec,
               "Precision in base ten as a positive integer (10^-p)")
            ->required();
    CLI::Option* fact_eff =
        app.add_option<int, int>(
               "--pollard-rho", factor_effort,
               "Sets MAX_ATTEMPTS_POLLARD_RHO, the effort "
               "taken to factorize candidate solutions (default=200)")
            ->default_val(MAX_ATTEMPTS_POLLARD_RHO);
    app.add_flag("-c, --check", check,
                 "Output bool that will be 1 if the op string matches the "
                 "input operator");
    app.add_flag(
        "-d, --details", details,
        "Output the particular value of the approximation including the power "
        "of root two in the denominator, the true error, and the T-count.");
    app.add_flag("-v, --verbose", verbose,
                 "Include additional output during runtime such as runtime "
                 "parameters and update on each step.");

    CLI11_PARSE(app, argc, argv);

    GridSynthOptions opt{prec, factor_effort, check, details, verbose};

    // Must initialize constants before parsing stdin using GMP
    MP_CONSTS = initialize_constants(opt.prec);
    auto program = parse_stdin("", true);
    if (program) {
        transformations::qasm_synth(*program, opt);
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
        return EXIT_FAILURE;
    }
}
