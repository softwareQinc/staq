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

#ifndef GRID_SYNTH_GRID_SYNTH_HPP_
#define GRID_SYNTH_GRID_SYNTH_HPP_

#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/matrix.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/types.hpp"

namespace staq {
namespace grid_synth {

/*! \brief Converts a GMP float to a string suitable for hashing. */
static str_t to_string(const mpf_class& x) {
    mp_exp_t exp;
    // Use base 32 to get a shorter string.
    // Truncate to keep only the significant figures.
    str_t s = x.get_str(exp, 32).substr(0, mpf_get_default_prec() / 5);
    return s + str_t(" ") + std::to_string(exp);
}

struct GridSynthOptions {
    long int prec;
    int factor_effort = MAX_ATTEMPTS_POLLARD_RHO;
    str_t tablefile = "";
    bool read = false;
    bool write = false;
    bool check = false;
    bool details = false;
    bool verbose = false;
    bool timer = false;
};

class GridSynthesizer {
  private:
    std::unordered_map<str_t, str_t> rz_approx_cache_;
    domega_matrix_table_t s3_table_;
    real_t eps_;
    bool check_;
    bool details_;
    bool verbose_;
    bool timer_;
    long long duration_;

    GridSynthesizer(domega_matrix_table_t s3_table, real_t eps, bool check,
                    bool details, bool verbose, bool timer)
        : rz_approx_cache_(), s3_table_(s3_table), eps_(eps), check_(check),
          details_(details), verbose_(verbose), timer_(timer), duration_(0) {}

  public:
    ~GridSynthesizer() {
        if (timer_) {
            std::cerr << std::fixed
                      << "Duration = " << (static_cast<double>(duration_) / 1e6)
                      << " seconds" << '\n';
        }
    }

    /*! \brief Find RZ-approximation for an angle. */
    str_t get_rz_approx(const real_t& angle) {
        if (verbose_)
            std::cerr << "Checking common cases..."
                      << "\n";
        str_t common_case = check_common_cases(angle / gmpf::gmp_pi(), eps_);
        if (common_case != "") {
            if (details_)
                std::cerr
                    << "Angle is multiple of pi/4, answer is known exactly"
                    << '\n';
            if (check_)
                std::cerr << "Check flag = " << 1 << '\n';
            return common_case;
        }
        if (verbose_)
            std::cerr << "No common cases found" << '\n';

        RzApproximation rz_approx;
        str_t op_str;
        if (timer_) {
            // If timer is enabled, don't check cache or produce debug
            // output. Just synthesize the angle.
            auto start = std::chrono::steady_clock::now();
            rz_approx = find_fast_rz_approximation(
                real_t(angle) * PI / real_t("-2"), eps_);
            op_str = synthesize(rz_approx.matrix(), s3_table_);
            auto end = std::chrono::steady_clock::now();
            duration_ += std::chrono::duration_cast<std::chrono::microseconds>(
                             end - start)
                             .count();
        } else {
            str_t angle_str = to_string(angle);
            if (verbose_)
                std::cerr << "Checking local cache..." << '\n';
            if (details_)
                std::cerr << "Angle has string representation " << angle_str
                          << '\n';
            if (rz_approx_cache_.count(angle_str)) {
                if (verbose_ || details_)
                    std::cerr << "Angle is found in local cache" << '\n';
                return rz_approx_cache_[angle_str];
            }

            if (verbose_)
                std::cerr
                    << "Running grid_synth to find new rz approximation..."
                    << '\n';
            RzApproximation rz_approx =
                find_fast_rz_approximation(angle / real_t("-2.0"), eps_);
            if (!rz_approx.solution_found()) {
                std::cerr << "No approximation found for RzApproximation. "
                             "Try changing factorization effort."
                          << '\n';
                exit(EXIT_FAILURE);
            }
            if (verbose_)
                std::cerr << "Approximation found. Synthesizing..." << '\n';
            op_str = synthesize(rz_approx.matrix(), s3_table_);

            if (verbose_)
                std::cerr << "Synthesis complete." << '\n';
            if (check_) {
                std::cerr << "Check flag = "
                          << (rz_approx.matrix() ==
                              domega_matrix_from_str(full_simplify_str(op_str)))
                          << '\n';
            }
            if (details_) {
                real_t scale = gmpf::pow(SQRT2, rz_approx.matrix().k());
                std::cerr << "angle = " << std::scientific << angle << '\n';
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
                std::cerr << "----" << '\n' << std::fixed;
            }
            rz_approx_cache_[angle_str] = op_str;
        }
        return op_str;
    }

    friend GridSynthesizer make_synthesizer(const GridSynthOptions& opt);
};

/*! \brief Initializes a GridSynthesizer object. */
GridSynthesizer make_synthesizer(const GridSynthOptions& opt) {
    domega_matrix_table_t s3_table;
    real_t eps;

    if (opt.read) {
        if (opt.verbose) {
            std::cerr << "Reading s3_table from " << opt.tablefile << '\n';
        }
        s3_table = read_s3_table(opt.tablefile);
    } else if (opt.write) {
        if (opt.verbose) {
            std::cerr << "Generating new table file and writing to "
                      << opt.tablefile << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(opt.tablefile, s3_table);
    } else if (std::ifstream(DEFAULT_TABLE_FILE)) {
        if (opt.verbose) {
            std::cerr << "Table file found at default location "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if (opt.verbose) {
            std::cerr << "Failed to find " << DEFAULT_TABLE_FILE
                      << ". Generating new table file and writing to "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE, s3_table);
    }

    MP_CONSTS = initialize_constants(opt.prec);
    eps = gmpf::pow(real_t(10), -opt.prec);
    MAX_ATTEMPTS_POLLARD_RHO = opt.factor_effort;

    if (opt.verbose) {
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

    return GridSynthesizer(std::move(s3_table), std::move(eps), opt.check,
                           opt.details, opt.verbose, opt.timer);
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_GRID_SYNTH_HPP_
