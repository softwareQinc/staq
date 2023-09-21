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

#include <CLI/CLI.hpp>
#include <iostream>

#include "grid_synth/gmp_functions.hpp"
#include "grid_synth/types.hpp"
#include "qasmtools/parser/parser.hpp"
#include "transformations/qasm_synth.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;
    using qasmtools::parser::parse_stdin;

    bool check = false, details = false, verbose = false;
    real_t eps;
    long int prec;
    int factor_effort;
    domega_matrix_table_t s3_table;
    str_t tablefile{};

    CLI::App app{"Grid Synthesis rx/ry/rz substitution"};

    // this interface is more or less identical to that of grid_synth.cpp
    // TODO: consider factoring out duplicated code?
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
    CLI::Option* read = app.add_option("-r, --read-table", tablefile,
                                       "Name of file containing s3_table");
    CLI::Option* write =
        app.add_option("-w, --write-table", tablefile,
                       "Name of table file to write s3_table to")
            ->excludes(read);
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

    transformations::QASMSynthOptions options{
        prec,        factor_effort, std::move(tablefile),
        (bool)*read, (bool)*write,  check,
        details,     verbose};

    auto program = parse_stdin("", true); // parse stdin using GMP
    if (program) {
        transformations::qasm_synth(*program, options);
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
        return 1;
    }
}
