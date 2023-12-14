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

#ifndef GRID_SYNTH_GMP_FUNCTIONS_HPP
#define GRID_SYNTH_GMP_FUNCTIONS_HPP

#include <cmath>
#include <iostream>
#include <string>

#include <gmpxx.h>

#include "staq/grid_synth/utils.hpp"

namespace staq {
namespace gmpf {

inline mpf_class gmp_abs(const mpf_class& x) { return sgn<mpf_class>(x) * x; }

// computes
inline mpf_class gmp_pi() {
    long int tol_exp = std::log10(2) * mpf_get_default_prec();
    mpf_class tol("1e-" + std::to_string(tol_exp));
    mpf_class three("3");
    mpf_class lasts("0");
    mpf_class t = three;
    mpf_class s("3");
    mpf_class n("1");
    mpf_class na("0");
    mpf_class d("0");
    mpf_class da("24");
    while (gmp_abs(s - lasts) > tol) {
        lasts = s;
        n = n + na;
        na = na + mpf_class("8");
        d = d + da;
        da = da + mpf_class("32");
        t = (t * n) / d;
        s += t;
    }
    return s;
}

inline mpz_class gmp_min(const mpz_class& x, const mpz_class& y) noexcept {
    return mpz_class(x * (x < y) + (1 - (x < y)) * y);
}

inline mpz_class gmp_max(const mpz_class& x, const mpz_class& y) noexcept {
    return mpz_class(x * (x > y) + (1 - (x > y)) * y);
}

inline mpf_class gmp_min(const mpf_class& x, const mpf_class& y) noexcept {
    return mpf_class(x * (x < y) + (1 - (x < y)) * y);
}

inline mpf_class gmp_max(const mpf_class& x, const mpf_class& y) noexcept {
    return mpf_class(x * (x > y) + (1 - (x > y)) * y);
}

inline mpz_class gmp_floor(const mpf_class& x) {
    mpf_class f;
    mpf_floor(f.get_mpf_t(), x.get_mpf_t());

    return mpz_class(f);
}

inline mpz_class gmp_ceil(const mpf_class& x) {
    mpf_class f;
    mpf_ceil(f.get_mpf_t(), x.get_mpf_t());

    return mpz_class(f);
}

inline mpz_class gmp_round(const mpf_class& x) {
    mpf_class f, c;

    mpf_floor(f.get_mpf_t(), x.get_mpf_t());
    mpf_ceil(c.get_mpf_t(), x.get_mpf_t());

    bool b = (abs(f - x) < abs(c - x));

    return mpz_class(b * f + (1 - b) * c);
}

inline mpf_class pow(const mpf_class& base, mpz_class exponent) {
    mpf_class output;

    mpf_pow_ui(output.get_mpf_t(), base.get_mpf_t(), exponent.get_ui());
    if (exponent < 0)
        return mpf_class(1) / output;

    return output;
}

inline mpf_class pow(const mpf_class& base, signed long int exponent) {
    mpf_class output;
    mpf_pow_ui(output.get_mpf_t(), base.get_mpf_t(), std::abs(exponent));

    if (exponent < 0)
        return mpf_class(1) / output;

    return output;
}

inline mpf_class gmp_leq(const mpf_class& lhs, const mpf_class& rhs) {
    long int tol_exp = std::log10(2) * lhs.get_prec();
    mpf_class tol = mpf_class("1e-" + std::to_string(tol_exp));
    return (lhs < rhs) || (abs(lhs - rhs) < tol);
}

inline mpf_class gmp_geq(const mpf_class& lhs, const mpf_class& rhs) {
    long int tol_exp = std::log10(2) * lhs.get_prec();
    mpf_class tol = mpf_class("1e-" + std::to_string(tol_exp));
    return (lhs > rhs) || (abs(lhs - rhs) < tol);
}

/*
 * Get the decimal part of an mpf_class element.
 */
inline mpf_class decimal_part(const mpf_class& x, mpz_class& intpart) {
    intpart = mpz_class(x);

    return x - intpart;
}

inline mpf_class log10(const mpf_class& x) {
    if (x <= 0) {
        std::cout << "mpf_class log(const mpf_class& x) expects x > 0"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    mpf_class m;
    mp_exp_t exp;
    std::string mantissa = x.get_str(exp);
    mantissa.insert(1, ".");
    int b = mpf_set_str(m.get_mpf_t(), mantissa.c_str(), 10);
    if (b == -1) {
        std::cout
            << "mpf_class log(mpf_class& x) failed to set m to mantissa string"
            << std::endl;
        exit(EXIT_FAILURE);
    }
    double l, d = mpf_get_d(m.get_mpf_t());
    l = std::log10(d);

    return mpf_class(l + exp - 1);
}

inline mpf_class log2(const mpf_class& x) {
    if (x <= 0) {
        std::cout << "mpf_class log(const mpf_class& x) expects x > 0"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    mpf_class m;
    mp_exp_t exp;
    std::string mantissa = x.get_str(exp);
    mantissa.insert(1, ".");
    int b = mpf_set_str(m.get_mpf_t(), mantissa.c_str(), 10);
    if (b == -1) {
        std::cout
            << "mpf_class log(mpf_class& x) failed to set m to mantissa string"
            << std::endl;
        exit(EXIT_FAILURE);
    }
    double l, d = mpf_get_d(m.get_mpf_t());
    l = std::log2(d);

    return mpf_class(l + exp - 1);
}

/*
 * Takes in an angle phi and reduces it to interval [-pi,pi] for evaluation.
 */
inline mpf_class reduce_angle(const mpf_class& phi) {
    mpf_class result = phi;
    mpf_class pi = gmp_pi();
    while (result > pi)
        result -= mpf_class("2") * pi;
    while (result < pi)
        result += mpf_class("2") * pi;
    return result;
}

// TODO improve accuracy of sin and cos by expanding about pi/2, pi/3, pi/4, and
// pi/6 depending on the input angle.
inline mpf_class sin(const mpf_class& theta) {
    long int initial_prec = theta.get_prec();
    long int tol_exp = std::log10(2) * initial_prec;
    mpf_class eps(("1e-" + std::to_string(tol_exp)));
    mpf_class phi = reduce_angle(theta);
    mpz_class i(1);
    mpf_class lasts(0);
    mpf_class s = phi;
    mpf_class fact(1);
    mpf_class num(phi);
    mpf_class sign(1);
    while (gmp_abs(s - lasts) > eps) {
        lasts = s;
        i += mpf_class("2");
        fact *= i * (i - mpf_class("1"));
        num *= phi * phi;
        sign *= mpf_class("-1");
        s += sign * (num / fact);
    }
    return s;
}

inline mpf_class cos(const mpf_class& theta) {
    // long int initial_prec = theta.get_prec();
    long int tol_exp = std::log10(2) * theta.get_prec();
    mpf_class eps(("1e-" + std::to_string(tol_exp)));
    mpf_class phi = reduce_angle(theta);
    mpz_class i(0);
    mpf_class lasts(0);
    mpf_class s(1);
    mpf_class fact(1);
    mpf_class num(1);
    mpf_class sign(1);
    while (gmp_abs(s - lasts) > eps) {
        lasts = s;
        i += mpf_class("2");
        fact *= i * (i - mpf_class("1"));
        num *= phi * phi;
        sign *= mpf_class("-1");
        s += sign * (num / fact);
    }
    return s;
}

inline mpf_class exp(const mpf_class& x) {
    if (x < 0)
        return 1 / gmpf::exp(-x);
    long int tol_exp = std::log10(2) * x.get_prec();
    mpf_class eps(("1e-" + std::to_string(tol_exp)));
    mpz_class i = 1;
    mpf_class s = 1;
    mpf_class term = x;
    // Use Taylor's remainder theorem bound on error
    while (gmp_abs(term * s) > eps) {
        s += term;
        i += 1;
        term *= x;
        term /= i;
    }
    return s;
}

inline mpf_class sqrt(const mpf_class& x) {
    mpf_class output;
    mpf_sqrt(output.get_mpf_t(), x.get_mpf_t());
    return output;
}

} // namespace gmpf
} // namespace staq

#endif // GRID_SYNTH_GMP_FUNCTIONS_HPP
