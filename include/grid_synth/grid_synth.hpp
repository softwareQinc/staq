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

#include "exact_synthesis.hpp"
#include "matrix.hpp"
#include "rz_approximation.hpp"
#include "s3_table.hpp"
#include "types.hpp"

namespace staq {
namespace grid_synth {

/* Converts a GMP float to a string suitable for hashing. */
static str_t to_string(const mpf_class& x) {
    mp_exp_t exp;
    // Use base 32 to get a shorter string; truncate the string
    // to keep only the significant figures.
    int sig_len = mpf_get_default_prec() / 5;
    if (x < 0)
        ++sig_len; // account for leading minus sign
    str_t s = x.get_str(exp, 32).substr(0, sig_len);
    return s + str_t(" ") + std::to_string(exp);
}

/* Options passed when constructing a GridSynthesizer. */
struct GridSynthOptions {
    long int prec; // Precision in base 10 as a positive integer (10^p)
    int factor_effort = MAX_ATTEMPTS_POLLARD_RHO;
    bool check = false;
    bool details = false;
    bool verbose = false;
    bool timer = false;
};

class GridSynthesizer {
  private:
    std::unordered_map<str_t, str_t> angle_cache_;
    const domega_matrix_table_t S3_TABLE;

    real_t eps_;
    bool check_;
    bool details_;
    bool verbose_;
    bool timer_;

    long long duration_;
    bool valid_;

    /* Construct GridSynthesizer objects using the make_synthesizer
     * factory function. 
     */
    GridSynthesizer(domega_matrix_table_t s3_table, real_t eps, bool check,
                    bool details, bool verbose, bool timer)
        : angle_cache_(), S3_TABLE(std::move(s3_table)),
          eps_(std::move(eps)), check_(check), details_(details),
          verbose_(verbose), timer_(timer), duration_(0), valid_(true) {}

  public:
    ~GridSynthesizer() {}

    double get_duration() const { return static_cast<double>(duration_) / 1e6; }
    bool is_valid() const { return valid_; }

    /*! \brief Find RZ-approximation for an angle. */
    str_t get_op_str(const real_t& angle) {
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
            op_str = synthesize(rz_approx.matrix(), S3_TABLE);
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
            if (angle_cache_.count(angle_str)) {
                if (verbose_ || details_)
                    std::cerr << "Angle is found in local cache" << '\n';
                return angle_cache_[angle_str];
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
            op_str = synthesize(rz_approx.matrix(), S3_TABLE);

            if (verbose_)
                std::cerr << "Synthesis complete." << '\n';
            bool good = (rz_approx.matrix() ==
                         domega_matrix_from_str(full_simplify_str(op_str)));
            valid_ = valid_ && good;
            valid_ = valid_ && (rz_approx.error() < eps_);

            if (check_)
                std::cerr << "Check flag = " << good << '\n';
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
            angle_cache_[angle_str] = op_str;
        }
        return op_str;
    }

    friend GridSynthesizer make_synthesizer(const GridSynthOptions& opt);
};

/*! \brief Initializes a GridSynthesizer object. */
inline GridSynthesizer make_synthesizer(const GridSynthOptions& opt) {
    domega_matrix_table_t s3_table = load_s3_table();

    real_t eps = gmpf::pow(real_t(10), -opt.prec);
    MP_CONSTS = initialize_constants(opt.prec);
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
