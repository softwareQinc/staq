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

#ifndef GRID_SYNTH_COMPLEX_HPP_
#define GRID_SYNTH_COMPLEX_HPP_

#include <iomanip>
#include <iostream>

#include <gmpxx.h>

#include "gmp_functions.hpp"

namespace staq {
namespace grid_synth {

template <typename T>
class complex {
  private:
    T a_;
    T b_;

  public:
    // Default constructor sets real and imaginary components to zero
    complex<T>() : a_(0), b_(0) {}
    complex<T>(T a, T b) : a_(a), b_(b) {}

    complex<T> conj() { return complex<T>(a_, -b_); }

    T a() const { return a_; }
    T b() const { return b_; }

    T real() const { return a_; }
    T imag() const { return b_; }

    T norm() const { return a_ * a_ + b_ * b_; }

    complex<T> operator*(complex<T> z) const {
        return complex<T>(a_ * z.a() - b_ * z.b(), a_ * z.b() + b_ * z.a());
    }

    complex<T> operator+(complex<T> z) const {
        return complex<T>(a_ + z.a(), b_ + z.b());
    }

    complex<T> operator-(complex<T> z) const {
        return complex<T>(a_ - z.a(), b_ - z.b());
    }

    complex<T> operator/(complex<T> z) const {
        return complex<T>((a_ * z.a() + b_ * z.b()) / z.norm(),
                          (b_ * z.a() - a_ * z.b()) / z.norm());
    }

    complex<T> operator+=(complex<T> z) {
        a_ += z.a();
        b_ += z.b();

        return (*this);
    }

    complex<T> operator-=(complex<T> z) {
        a_ -= z.a();
        b_ -= z.b();

        return (*this);
    }

    complex<T> operator*=(complex<T> z) {
        T olda = a_;
        T oldb = b_;

        a_ = olda * z.a() - oldb * z.b();
        b_ = olda * z.b() + oldb * z.a();

        return (*this);
    }

    complex<T> operator/=(complex<T> z) {
        T olda = a_;
        T oldb = b_;
        a_ = (olda * z.a() + oldb * z.b()) / z.norm();
        b_ = (oldb * z.a() - olda * z.b()) / z.norm();

        return (*this);
    }
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const complex<T>& z) {
    os << "(" << z.a() << "," << z.b() << ")";
    return os;
}

template <typename T>
inline complex<T> operator+(complex<T> z, T x) {
    return complex<T>(z.a() + x, z.b());
}

template <typename T>
inline complex<T> operator+(T x, complex<T> z) {
    return complex<T>(z.a() + x, z.b());
}

template <typename T>
inline complex<T> operator+(double x, complex<T> z) {
    return complex<T>(z.a() + T(x), z.b());
}

template <typename T>
inline complex<T> operator+(complex<T> z, double x) {
    return complex<T>(z.a() + T(x), z.b());
}

template <typename T>
inline complex<T> operator+(complex<T> z, long long int x) {
    return complex<T>(z.a() + T(x), z.b());
}

template <typename T>
inline complex<T> operator+(long long int x, complex<T> z) {
    return complex<T>(z.a() + T(x), z.b());
}

template <typename T>
inline complex<T> operator*(complex<T> z, T x) {
    return complex<T>(z.a() * T(x), z.b() * T(x));
}

template <typename T>
inline complex<T> operator*(T x, complex<T> z) {
    return complex<T>(z.a() * T(x), z.b() * T(x));
}

template <typename T>
inline complex<T> operator*(complex<T> z, double x) {
    return complex<T>(z.a() * T(x), z.b() * T(x));
}

inline complex<mpf_class> operator*(complex<mpf_class> z, complex<double> x) {
    return complex<mpf_class>(
        z.a() * mpf_class(x.a()) - z.b() * mpf_class(x.b()),
        z.a() * mpf_class(x.b()) + z.b() * mpf_class(x.a()));
}

inline complex<mpf_class> operator*(complex<double> x, complex<mpf_class> z) {
    return complex<mpf_class>(z.a() * x.a() - z.b() * x.b(),
                              z.a() * x.b() + z.b() * x.a());
}

inline mpf_class abs(const complex<mpf_class>& z) {
    return gmpf::sqrt(z.real() * z.real() + z.imag() * z.imag());
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_COMPLEX_HPP_
