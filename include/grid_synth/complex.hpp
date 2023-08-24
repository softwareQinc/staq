#ifndef COMPLEX_HPP
#define COMPLEX_HPP

#include <gmpxx.h>
#include <iomanip>
#include <iostream>

namespace staq{
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

    complex<T> operator+=(complex<T> z) const {
        a_ += z.a();
        b_ += z.b();

        return (*this);
    }

    complex<T> operator-=(complex<T> z) const {
        a_ -= z.a();
        b_ -= z.b();

        return (*this);
    }

    complex<T> operator/=(complex<T> z) const {
        a_ /= z.a();
        b_ /= z.b();

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

} // namespace grid_synth
} // namespace staq

#endif // COMPLEX_HPP
