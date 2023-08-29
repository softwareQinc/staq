#ifndef GMP_FUNCTIONS_HPP
#define GMP_FUNCTIONS_HPP

#include <cmath>
#include <gmpxx.h>

#include "complex.hpp"
#include "constants.hpp"
#include "utils.hpp"

namespace staq {
namespace grid_synth {

// TODO add high precision sine and cosine functions

inline mpf_class gmp_pi(const mpf_class& tol) {
    using namespace std;
    real_t three("3");
    real_t lasts("0");
    real_t t = three;
    real_t s("3");
    real_t n("1");
    real_t na("0");
    real_t d("0");
    real_t da("24");
    while (abs(s - lasts) > tol) {
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

inline mpz_class min(const mpz_class& x, const mpz_class& y) noexcept {
    return mpz_class(x * (x < y) + (1 - (x < y)) * y);
}

inline mpz_class max(const mpz_class& x, const mpz_class& y) noexcept {
    return mpz_class(x * (x > y) + (1 - (x > y)) * y);
}

inline mpf_class min(const mpf_class& x, const mpf_class& y) noexcept {
    return mpf_class(x * (x < y) + (1 - (x < y)) * y);
}

inline mpf_class max(const mpf_class& x, const mpf_class& y) noexcept {
    return mpf_class(x * (x > y) + (1 - (x > y)) * y);
}

inline mpz_class floor(const mpf_class& x) {
    mpf_class f;
    mpf_floor(f.get_mpf_t(), x.get_mpf_t());

    return mpz_class(f);
}

inline mpz_class ceil(const mpf_class& x) {
    mpf_class f;
    mpf_ceil(f.get_mpf_t(), x.get_mpf_t());

    return mpz_class(f);
}

inline mpz_class round(const mpf_class& x) {
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
    mpf_pow_ui(output.get_mpf_t(), base.get_mpf_t(), abs(exponent));

    if (exponent < 0)
        return mpf_class(1) / output;

    return output;
}

inline mpf_class fleq(const mpf_class& lhs, const mpf_class& rhs,
                      const mpf_class& tol = TOL) {
    return (lhs < rhs) or (abs(lhs - rhs) < TOL);
}

inline mpf_class fgeq(const mpf_class& lhs, const mpf_class& rhs,
                      const mpf_class& tol = TOL) {
    return (lhs > rhs) or (abs(lhs - rhs) < TOL);
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
 * Takes in an angle phi and reduces it to the rand [-pi,pi] for evaluation.
 */
inline mpf_class reduce_angle(const mpf_class& phi) {
    mpf_class result = phi;
    while (result > PI)
        result -= 2 * PI;
    while (result < -PI)
        result += 2 * PI;
    return result;
}

inline mpf_class sin(const mpf_class& theta, const mpf_class& tol = TOL) {
    real_t phi = reduce_angle(theta);
    mpz_class i = 1;
    mpf_class lasts = 0;
    mpf_class s = phi;
    mpf_class fact = 1;
    mpf_class num = phi;
    mpf_class sign = 1;
    while (abs(s - lasts) > tol) {
        lasts = s;
        i += 2;
        fact *= i * (i - 1);
        num *= phi * phi;
        sign *= -1;
        s += sign * (num / fact);
    }
    return s;
}

inline mpf_class cos(const mpf_class& theta, const mpf_class& tol = TOL) {
    real_t phi = reduce_angle(theta);
    mpz_class i = 0;
    mpf_class lasts = 0;
    mpf_class s = 1;
    mpf_class fact = 1;
    mpf_class num = 1;
    mpf_class sign = 1;
    while (abs(s - lasts) > tol) {
        lasts = s;
        i += 2;
        fact *= i * (i - 1);
        num *= phi * phi;
        sign *= -1;
        s += sign * (num / fact);
    }
    return s;
}

inline mpf_class abs(const complex<mpf_class>& z) {
    return sqrt(z.real() * z.real() + z.imag() * z.imag());
}

inline mpf_class abs(const mpf_class& x) { return sgn<mpf_class>(x) * x; }

inline mpf_class sqrt(const mpf_class& x) {
    mpf_class output;
    mpf_sqrt(output.get_mpf_t(), x.get_mpf_t());
    return output;
}

} // namespace grid_synth
} // namespace staq

#endif // GMP_FUNCTIONS_HPP
