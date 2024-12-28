/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2025 softwareQ Inc. All rights reserved.
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

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "third_party/CLI/CLI.hpp"

#include "staq/grid_synth/exact_synthesis.hpp"
#include "staq/grid_synth/grid_synth.hpp"
#include "staq/grid_synth/regions.hpp"
#include "staq/grid_synth/rz_approximation.hpp"
#include "staq/grid_synth/types.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;

    bool check = false, details = false, verbose = false, timer = false;
    std::vector<std::string> thetas;
    long int prec;
    int factor_effort;

    CLI::App app{"Grid Synthesis"};

    CLI::Option* thetas_op =
        app.add_option("theta", thetas, "Z-rotation angle(s) in units of PI")
            ->required();

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
    app.add_flag("--time", timer, "Time program");

    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        std::cerr << thetas.size() << " angle(s) read." << '\n';
    }

    GridSynthOptions opt{prec, factor_effort, check, details, verbose, timer};
    GridSynthesizer synthesizer = make_synthesizer(opt);

    if (*prec_opt && *thetas_op) {
        std::random_device rd;
        random_numbers.seed(rd());

        for (const auto& angle : thetas) {
            real_t gmp_angle;
            try {
                gmp_angle = real_t(angle);
            } catch (std::invalid_argument& e) {
                std::cerr << "Invalid angle provided: " << angle << std::endl;
                return EXIT_FAILURE;
            }
            str_t op_str = synthesizer.get_op_str(gmp_angle * gmpf::gmp_pi());
            for (char c : op_str) {
                std::cout << c << ' ';
            }
            std::cout << '\n';
        }
    }

    if (timer) {
        std::cerr << std::fixed << "Duration = " << synthesizer.get_duration()
                  << " seconds" << '\n';
    }
}
