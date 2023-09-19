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
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/regions.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/types.hpp"

int main(int argc, char** argv) {

    using namespace staq;
    using namespace grid_synth;

    bool check = false, details = false, verbose = false, timer = false;
    real_t theta, eps;
    std::vector<std::string> thetas;
    std::vector<long int> prec_lst;
    long int prec;
    int factor_effort;
    domega_matrix_table_t s3_table;
    str_t tablefile{};

    CLI::App app{"Grid Synthesis"};

    CLI::Option* thetas_op =
        app.add_option("-t, --theta", thetas,
                       "Z-rotation angle(s) in units of PI")
            ->required();
    CLI::Option* prec_opt =
        app.add_option<long int, int>(
               "-p, --precision", prec,
               "Precision in base ten as a positive integer (10^-p)")
            ->required();
    CLI::Option* fact_eff = app.add_option<int, int>(
        "--pollard-rho", factor_effort,
        "Sets MAX_ATTEMPTS_POLLARD_RHO, the effort "
        "taken to factorize candidate solutions (default=200)");
    CLI::Option* read = app.add_option("-r, --read-table", tablefile,
                                       "Name of file containing s3 table");
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
    app.add_flag("--time", timer, "Time program");

    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        std::cerr << thetas.size() << " angles read." << '\n';
    }

    if (*read) {
        if (verbose) {
            std::cerr << "Reading s3_table from " << tablefile << '\n';
        }
        s3_table = read_s3_table(tablefile);
    } else if (*write) {
        if (verbose) {
            std::cerr << "Generating new table file and writing to "
                      << tablefile << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(tablefile, s3_table);
    } else if (std::ifstream(DEFAULT_TABLE_FILE)) {
        if (verbose) {
            std::cerr << "Table file found at default location "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if (verbose) {
            std::cerr << "Failed to find " << DEFAULT_TABLE_FILE
                      << ". Generating new table file and writing to "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE, s3_table);
    }

    MP_CONSTS = initialize_constants(prec);
    eps = gmpf::pow(real_t(10), -prec);

    if (*fact_eff) {
        MAX_ATTEMPTS_POLLARD_RHO = factor_effort;
    }

    if (verbose) {
        std::cerr << "Runtime Parameters" << '\n';
        std::cerr << "------------------" << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "TOL (Tolerance for float equality) " << std::setw(1)
                  << ": " << std::setw(3 * COLW) << std::left << std::scientific
                  << TOL << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "KMIN (Minimum scaling exponent) " << std::setw(1) << ": "
                  << std::setw(3 * COLW) << std::left << std::fixed << KMIN
                  << '\n';
        std::cerr << std::setw(2 * COLW) << std::left
                  << "KMAX (Maximum scaling exponent) " << std::setw(1) << ": "
                  << std::setw(3 * COLW) << std::left << std::fixed << KMAX
                  << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "MAX_ATTEMPTS_POLLARD_RHO (How hard we try to factor) "
                  << std::setw(1) << ": " << std::setw(3 * COLW) << std::left
                  << MAX_ATTEMPTS_POLLARD_RHO << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "MAX_ITERATIONS_FERMAT_TEST (How hard we try to check "
                     "primality) "
                  << std::setw(1) << ": " << std::setw(3 * COLW) << std::left
                  << MAX_ITERATIONS_FERMAT_TEST << '\n';
    }
    std::cerr << std::scientific;

    long long duration = 0;
    if (*prec_opt && *thetas_op) {
        std::random_device rd;
        random_numbers.seed(rd());

        for (const auto& angle : thetas) {
            str_t common_case = check_common_cases(real_t(angle), eps);
            if (verbose)
                std::cerr << "Checking common cases..." << '\n';
            if (common_case != "") {
                if (details)
                    std::cerr
                        << "Angle is multiple of pi/4, answer is known exactly"
                        << '\n';
                if (check) {
                    std::cerr << "Check flag = " << 1 << '\n';
                }
                std::cout << common_case << '\n';
                return 0;
            }
            if (verbose)
                std::cerr << "No common cases found" << '\n';

            RzApproximation rz_approx;
            if (timer) {
                auto start = std::chrono::steady_clock::now();
                rz_approx = find_fast_rz_approximation(
                    real_t(angle) * PI / real_t("-2"), eps);
                str_t op_str = synthesize(rz_approx.matrix(), s3_table);
                auto end = std::chrono::steady_clock::now();
                duration +=
                    std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                          start)
                        .count();
            } else {
                if (verbose) {
                    std::cerr << "----\n";
                    std::cerr << "Finding approximation for angle = " << (angle)
                              << "..." << '\n';
                }
                rz_approx = find_fast_rz_approximation(
                    real_t(angle) * PI / real_t("-2"), eps);
                if (!rz_approx.solution_found()) {
                    std::cerr << "No approximation found for RzApproximation. "
                                 "Try changing factorization effort."
                              << '\n';
                    return EXIT_FAILURE;
                }
                if (verbose) {
                    std::cerr << "Approximation found. Synthesizing..." << '\n';
                }
                str_t op_str = synthesize(rz_approx.matrix(), s3_table);
                if (verbose) {
                    std::cerr << "Synthesis complete." << '\n';
                }

                if (check) {
                    std::cerr
                        << "Check flag = "
                        << (rz_approx.matrix() ==
                            domega_matrix_from_str(full_simplify_str(op_str)))
                        << '\n';
                }

                if (details) {
                    real_t scale = gmpf::pow(SQRT2, rz_approx.matrix().k());
                    std::cerr << "angle = " << angle << '\n';
                    std::cerr << rz_approx.matrix();
                    std::cerr << "u decimal value = "
                              << "("
                              << rz_approx.matrix().u().decimal().real() / scale
                              << ","
                              << rz_approx.matrix().u().decimal().imag() / scale
                              << ")" << '\n';
                    std::cerr << "t decimal value = "
                              << "("
                              << rz_approx.matrix().t().decimal().real() / scale
                              << ","
                              << rz_approx.matrix().t().decimal().imag() / scale
                              << ")" << '\n';
                    std::cerr << "error = " << rz_approx.error() << '\n';
                    str_t simplified = full_simplify_str(op_str);
                    std::string::difference_type n =
                        count(simplified.begin(), simplified.end(), 'T');
                    std::cerr << "T count = " << n << '\n';
                    std::cerr << "----" << '\n';
                }

                for (auto& ch : full_simplify_str(op_str)) {
                    std::cout << ch << " ";
                }
                std::cout << '\n';
            }
        }
    }

    if (timer) {
        std::cerr << "Duration = " << (static_cast<double>(duration) / 1e6)
                  << '\n';
    }
}
