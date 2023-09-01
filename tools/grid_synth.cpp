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
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/types.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;
    using namespace std;

    bool check, details, verbose, timer, more_verbose;
    real_t theta, eps;
    vector<string> thetas;
    vector<long int> prec_lst;
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
    app.add_flag("-V, --more-verbose", more_verbose,
                 "Output update on approximation function");
    app.add_flag("--time", timer, "Time program");

    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        cout << thetas.size() << " angles read." << '\n';
    }

    if (more_verbose) {
        verbose = true;
    }

    if (*read) {
        if (verbose) {
            cout << "Reading s3_table from " << tablefile << '\n';
        }
        s3_table = read_s3_table(tablefile);
    } else if (*write) {
        if (verbose) {
            cout << "Generating new table file and writing to " << tablefile
                 << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(tablefile, s3_table);
    } else if (ifstream(DEFAULT_TABLE_FILE)) {
        if (verbose) {
            cout << "Table file found at default location "
                 << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if (verbose) {
            cout << "Failed to find " << DEFAULT_TABLE_FILE
                 << ". Generating new table file and writing to "
                 << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE, s3_table);
    }

    DEFAULT_GMP_PREC = 4 * prec + 19;
    mpf_set_default_prec(log2(10) * DEFAULT_GMP_PREC);
    TOL = pow(real_t(10), -DEFAULT_GMP_PREC + 2);
    PI = gmp_pi(TOL);
    SQRT2 = sqrt(real_t(2));
    INV_SQRT2 = real_t(real_t(1) / SQRT2);
    HALF_INV_SQRT2 = real_t(real_t(1) / (real_t(2) * SQRT2));
    OMEGA = cplx_t(INV_SQRT2, INV_SQRT2);
    OMEGA_CONJ = cplx_t(INV_SQRT2, -INV_SQRT2);
    LOG_LAMBDA = log10(LAMBDA.decimal());
    SQRT_LAMBDA = sqrt(LAMBDA.decimal());
    SQRT_LAMBDA_INV = sqrt(LAMBDA_INV.decimal());
    Im = cplx_t(real_t(0), real_t(1));
    eps = pow(real_t(10), -prec);

    if (*fact_eff) {
        MAX_ATTEMPTS_POLLARD_RHO = factor_effort;
    }

    if (verbose) {
        cout << "Runtime Parameters" << '\n';
        cout << "------------------" << '\n';
        cout << setw(3 * COLW) << left << "TOL (Tolerance for float equality) "
             << setw(1) << ": " << setw(3 * COLW) << left << scientific << TOL
             << '\n';
        cout << setw(3 * COLW) << left << "KMIN (Minimum scaling exponent) "
             << setw(1) << ": " << setw(3 * COLW) << left << fixed << KMIN
             << '\n';
        cout << setw(2 * COLW) << left << "KMAX (Maximum scaling exponent) "
             << setw(1) << ": " << setw(3 * COLW) << left << fixed << KMAX
             << '\n';
        cout << setw(3 * COLW) << left
             << "MAX_ATTEMPTS_POLLARD_RHO (How hard we try to factor) "
             << setw(1) << ": " << setw(3 * COLW) << left
             << MAX_ATTEMPTS_POLLARD_RHO << '\n';
        cout << setw(3 * COLW) << left
             << "MAX_ITERATIONS_FERMAT_TEST (How hard we try to check "
                "primality) "
             << setw(1) << ": " << setw(3 * COLW) << left
             << MAX_ITERATIONS_FERMAT_TEST << endl;
    }
    cout << scientific;

    long long duration = 0;
    if (*prec_opt && *thetas_op) {
        random_device rd;
        random_numbers.seed(rd());

        for (const auto& angle : thetas) {
            RzApproximation rz_approx;
            if (timer) {
                auto start = chrono::steady_clock::now();
                rz_approx = find_fast_rz_approximation(
                    real_t(angle) * PI / real_t("-2"), eps);
                str_t op_str = synthesize(rz_approx.matrix(), s3_table);
                auto end = chrono::steady_clock::now();
                duration +=
                    chrono::duration_cast<chrono::microseconds>(end - start)
                        .count();
            } else {
                if (verbose) {
                    cout << "----\n";
                    cout << "Finding approximation for angle = " << (angle)
                         << "..." << '\n';
                }
                if (more_verbose) {
                    rz_approx = verbose_find_fast_rz_approximation(
                        real_t(angle) * PI / real_t("-2"), eps);
                } else {
                    rz_approx = find_fast_rz_approximation(
                        real_t(angle) * PI / real_t("-2"), eps);
                }
                if (!rz_approx.solution_found()) {
                    cout << "No approximation found for RzApproximation. Try "
                            "changing factorization effort."
                         << '\n';
                    return EXIT_FAILURE;
                }
                if (verbose) {
                    cout << "Approximation found. Synthesizing..." << '\n';
                }
                str_t op_str = synthesize(rz_approx.matrix(), s3_table);
                if (verbose) {
                    cout << "Synthesis complete." << '\n';
                }

                if (check) {
                    cout << "Check flag = "
                         << (rz_approx.matrix() ==
                             domega_matrix_from_str(full_simplify_str(op_str)))
                         << '\n';
                }

                if (details) {
                    real_t scale = pow(SQRT2, rz_approx.matrix().k());
                    cout << "angle = " << angle << '\n';
                    cout << rz_approx.matrix();
                    cout << "u decimal value = "
                         << "("
                         << rz_approx.matrix().u().decimal().real() / scale
                         << ","
                         << rz_approx.matrix().u().decimal().imag() / scale
                         << ")" << '\n';
                    cout << "t decimal value = "
                         << "("
                         << rz_approx.matrix().t().decimal().real() / scale
                         << ","
                         << rz_approx.matrix().t().decimal().imag() / scale
                         << ")" << '\n';
                    cout << "error = " << rz_approx.error() << '\n';
                    str_t simplified = full_simplify_str(op_str);
                    string::difference_type n =
                        count(simplified.begin(), simplified.end(), 'T');
                    cout << "T count = " << n << '\n';
                    cout << "----" << '\n';
                }

                for (auto& ch : full_simplify_str(op_str)) {
                    cout << ch << " ";
                }
                cout << endl;
            }
        }
    }

    if (timer) {
        cout << "Duration = " << (static_cast<double>(duration) / 1e6) << endl;
    }
}
