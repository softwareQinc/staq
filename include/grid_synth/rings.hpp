#ifndef RINGS_HPP
#define RINGS_HPP

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "constants.hpp"
#include "gmp_functions.hpp"
#include "types.hpp"

namespace staq {
namespace grid_synth {

/*
 * An implementation of the ring of quadratic integers with radicand 2.
 * In this class we refer to the conjugate operator as dot. That is,
 *
 *    dot(a + b\sqrt{2}) = a - b\sqrt{2}
 *
 * This is to avoid confusion with complex conjugation.
 */
class ZSqrt2 {

  private:
    int_t a_;
    int_t b_;

  public:
    explicit ZSqrt2(){};

    explicit ZSqrt2(const int_t& a) : a_(a), b_(0) {}

    ZSqrt2(const int_t& a, const int_t& b) : a_(a), b_(b) {}

    int_t a() const { return a_; }
    int_t b() const { return b_; }

    real_t decimal() const { return a_ + (b_ * SQRT2); }
    real_t decimal_dot() const { return (a_ - b_ * SQRT2); }

    int_t norm() const { return a_ * a_ - (2 * b_ * b_); }
    ZSqrt2 dot() const { return ZSqrt2(a_, -b_); }

    std::string get_string() {
        std::stringstream ss;
        ss << "(" << a_ << "," << b_ << ")";
        return ss.str();
    }

    ZSqrt2 self_sqrt() const {
        using namespace std;
        real_t a, b;
        int_t b_squared_plus = (a_ + sqrt((*this).norm())) / 4;
        int_t b_squared_minus = (a_ - sqrt((*this).norm())) / 4;

        if (pow(round(sqrt(b_squared_plus)), 2) == b_squared_plus)
            b = sqrt(b_squared_plus);
        else if (pow(round(sqrt(b_squared_minus)), 2) == b_squared_minus)
            b = sqrt(b_squared_minus);
        else {
            std::cout << "(" << a_ << "," << b_ << ")"
                      << " isn't a square." << std::endl;
            exit(EXIT_FAILURE);
        }
        if (abs(b) < TOL) {
            a = sqrt(a_);
            if (int_t(a) * int_t(a) != a_) {
                std::cout << "(" << a_ << "," << b_ << ")"
                          << " isn't a square." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else
            a = b_ / (2 * b);

        return ZSqrt2(round(a), round(b));
    }

    // Arithmetic operators
    // ====================
    ZSqrt2 operator+(const ZSqrt2& Z) const {
        return ZSqrt2(a_ + Z.a(), b_ + Z.b());
    }

    ZSqrt2 operator-(const ZSqrt2& Z) const {
        return ZSqrt2(a_ - Z.a(), b_ - Z.b());
    }

    ZSqrt2 operator*(const ZSqrt2& Z) const {
        return ZSqrt2(a_ * Z.a() + 2 * b_ * Z.b(), (a_ * Z.b() + b_ * Z.a()));
    }

    /*
     * For a / b finds the largest q such that a > bq
     */
    ZSqrt2 operator/(const ZSqrt2& Z) const {
        using namespace std;
        real_t mag = Z.norm();
        int_t a = round(real_t(a_ * Z.a() - 2 * b_ * Z.b()) / mag);
        int_t b = round(real_t(b_ * Z.a() - a_ * Z.b()) / mag);
        return ZSqrt2(a, b);
    }

    /*
     * For a % b finds q and r such that a = bq + r.
     */
    ZSqrt2 operator%(const ZSqrt2& Z) const {
        return (*this) - (*this / Z) * Z;
    }

    // Assignment and compound assignment operators
    // ===========================================
    ZSqrt2& operator+=(const ZSqrt2& Z) {
        a_ += Z.a();
        b_ += Z.b();
        return *this;
    }

    ZSqrt2& operator-=(const ZSqrt2& Z) {
        a_ -= Z.a();
        b_ -= Z.b();
        return *this;
    }

    ZSqrt2& operator*=(const ZSqrt2& Z) {
        int_t olda = a_;
        int_t oldb = b_;
        a_ = olda * Z.a() + 2 * oldb * Z.b();
        b_ = olda * Z.b() + oldb * Z.a();
        return *this;
    }

    void print_decimal(const int colw = COLW, const int prec = PREC) {
        std::cout << std::setw(colw) << std::left << std::setfill(' ')
                  << this->get_string() << std::setw(colw) << std::left
                  << std::setprecision(prec) << this->decimal()
                  << std::setw(colw) << std::left << std::setprecision(prec)
                  << this->decimal_dot() << std::endl;
    }

}; // class ZSqrt2

inline ZSqrt2 pow(const ZSqrt2& Z, const int_t& k) {
    try {
        if (k < 0)
            throw std::invalid_argument("Operator ^ for ZSqrt2 expects k > 0");
    } catch (std::invalid_argument const& ex) {
        std::cout << ex.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    ZSqrt2 result(1, 0);

    for (int_t i = 0; i < k; i++) {
        result *= Z;
    }

    return result;
}

// Non-member operator overrides for ZSqrt2
// ========================================
inline std::ostream& operator<<(std::ostream& os, const ZSqrt2& Z) {
    os << "(" << Z.a() << "," << Z.b() << ")";
    return os;
}

inline ZSqrt2 operator*(const ZSqrt2& Z, const int_t& c) {
    return ZSqrt2(Z.a() * c, Z.b() * c);
}

inline ZSqrt2 operator*(const int_t& c, const ZSqrt2 Z) {
    return ZSqrt2(Z.a() * c, Z.b() * c);
}

inline bool operator==(const ZSqrt2& Y, const ZSqrt2& Z) {
    return (Y.a() == Z.a()) && (Y.b() == Z.b());
}

inline bool operator!=(const ZSqrt2& Y, const ZSqrt2& Z) {
    return (Y.a() != Z.a()) || (Y.b() != Z.b());
}

inline bool operator>=(const ZSqrt2& Y, const ZSqrt2& Z) {
    return (Y.decimal() > Z.decimal()) || (Y == Z);
}

inline bool operator<=(const ZSqrt2& Y, const ZSqrt2& Z) {
    return (Y.decimal() < Z.decimal()) || (Y == Z);
}

inline bool operator>(const ZSqrt2& Y, const ZSqrt2& Z) {
    return Y.decimal() > Z.decimal();
}

inline bool operator<(const ZSqrt2& Y, const ZSqrt2& Z) {
    return Y.decimal() < Z.decimal();
}

inline bool operator>(const ZSqrt2 Z, const real_t& x) {
    return Z.decimal() > x;
}

inline bool operator>(const real_t& x, const ZSqrt2& Z) {
    return x > Z.decimal();
}

inline bool operator<(const ZSqrt2& Z, const real_t& x) {
    return Z.decimal() < x;
}

inline bool operator<(const real_t& x, const ZSqrt2& Z) {
    return x < Z.decimal();
}

/**
 * An implementation of the ring of cyclotomic integers of degree 8. Each
 * element is represented in two forms. First by two quadratic integers
 * with radicand 2, alpha and beta, and a boolean. Thus we have,
 *
 *    u = alpha_ + beta_*i + w_*OMEGA
 *
 *  where w_ is zero or one, and OMEGA is the constant (1+i)/SQRT2.
 *  Second in the standard form,
 *
 *    u = a_*OMEGA^3 + b_*OMEGA^2 + c_*OMEGA + d
 */
class ZOmega {
  private:
    int_t a_;
    int_t b_;
    int_t c_;
    int_t d_;

    ZSqrt2 alpha_;
    ZSqrt2 beta_;
    cplx_t w_;

  public:
    explicit ZOmega(const int_t& d)
        : a_(0), b_(0), c_(0), d_(d), w_(cplx_t(0, 0)) {
        int_t alpha1, alpha2, beta1, beta2;

        alpha1 = d_;
        alpha2 = (c_ - a_ - w_.real()) / 2;
        beta1 = (b_);
        beta2 = (c_ + a_ - w_.real()) / 2;

        alpha_ = ZSqrt2(alpha1, alpha2);
        beta_ = ZSqrt2(beta1, beta2);
    }

    ZOmega(const int_t& a, const int_t& b, const int_t& c, const int_t& d)
        : a_(a), b_(b), c_(c), d_(d) {
        int_t alpha1, alpha2, beta1, beta2;

        w_ = cplx_t((c_ + a_) % 2, 0.0);

        alpha1 = d_;
        alpha2 = (c_ - a_ - w_.real()) / 2;
        beta1 = (b_);
        beta2 = (c_ + a_ - w_.real()) / 2;

        alpha_ = ZSqrt2(alpha1, alpha2);
        beta_ = ZSqrt2(beta1, beta2);
    }

    ZOmega(const ZSqrt2& alpha, const ZSqrt2& beta, const bool& w)
        : alpha_(alpha), beta_(beta), w_(cplx_t(w, 0.0)) {
        a_ = beta_.b() - alpha_.b();
        b_ = beta.a();
        c_ = beta_.b() + alpha_.b() + w;
        d_ = alpha.a();
    }

    int_t a() const { return a_; }
    int_t b() const { return b_; }
    int_t c() const { return c_; }
    int_t d() const { return d_; }

    ZSqrt2 alpha() const { return alpha_; }
    ZSqrt2 beta() const { return beta_; }

    /*
     * For an element u, returns the ring norm u^dagger * u
     */
    ZSqrt2 norm() const {
        return ZSqrt2(a_ * a_ + b_ * b_ + c_ * c_ + d_ * d_,
                      c_ * b_ + d_ * c_ + b_ * a_ - a_ * d_);
    }

    bool is_reducible() {
        if (((a_ + c_) % 2 == 0) && ((b_ + d_) % 2 == 0))
            return true;
        return false;
    }

    ZOmega reduce() {
        assert(this->is_reducible());

        return ZOmega((b_ - d_) / 2, (a_ + c_) / 2, (b_ + d_) / 2,
                      (c_ - a_) / 2);
    }

    bool w() const { return round(w_.real()) != 0; }

    ZOmega dot() const { return ZOmega(-a_, b_, -c_, d_); }
    ZOmega conj() const { return ZOmega(-c_, -b_, -a_, d_); }

    real_t real() const {
        return (alpha_.decimal() + beta_.decimal() * Im + w_ * OMEGA).real();
    }
    real_t imag() const {
        return (alpha_.decimal() + beta_.decimal() * Im + w_ * OMEGA).imag();
    }

    cplx_t decimal() const {
        return alpha_.decimal() + beta_.decimal() * Im + w_ * OMEGA;
    }

    ZSqrt2 to_zsqrt2() {
        if (b_ != 0) {
            std::cout << "ZOmega method to_zsqrt2 expects b_ == 0" << std::endl;
            exit(EXIT_FAILURE);
        }
        return ZSqrt2(d_, c_);
    }

    std::string get_standard_string() {
        std::stringstream ss;
        ss << "(" << a_ << "," << b_ << "," << c_ << "," << d_ << ")";

        return ss.str();
    }

    std::string get_zsqrt2_string() {
        std::stringstream ss;

        ss << "(" << alpha_.a() << "," << alpha_.b() << "," << beta_.a() << ","
           << beta_.b() << "," << int_t(w_.real()) << ")";

        return ss.str();
    }

    // Arithmetic operators
    // ====================
    ZOmega operator+(const ZOmega& Z) const {
        return ZOmega(a_ + Z.a(), b_ + Z.b(), c_ + Z.c(), d_ + Z.d());
    }

    ZOmega operator-(const ZOmega& Z) const {
        return ZOmega(a_ - Z.a(), b_ - Z.b(), c_ - Z.c(), d_ - Z.d());
    }

    ZOmega operator-() const { return ZOmega(-a_, -b_, -c_, -d_); }

    ZOmega operator*(const ZOmega& Z) const {
        return ZOmega(a_ * Z.d() + b_ * Z.c() + c_ * Z.b() + d_ * Z.a(),
                      -a_ * Z.a() + b_ * Z.d() + c_ * Z.c() + d_ * Z.b(),
                      -a_ * Z.b() - b_ * Z.a() + c_ * Z.d() + d_ * Z.c(),
                      -a_ * Z.c() - b_ * Z.b() - c_ * Z.a() + d_ * Z.d());
    }

    // Assignment and compound assignment operators
    // ===========================================
    ZOmega& operator+=(const ZOmega& Z) {
        a_ += Z.a();
        b_ += Z.b();
        c_ += Z.c();
        d_ += Z.d();

        ZSqrt2 shift(0, round(w_.real()) != 0 && Z.w());

        alpha_ += Z.alpha() + shift;
        beta_ += Z.beta() + shift;

        return *this;
    }

    ZOmega& operator-=(const ZOmega& Z) {
        using namespace std;
        a_ -= Z.a();
        b_ -= Z.b();
        c_ -= Z.c();
        d_ -= Z.d();

        ZSqrt2 shift(0, round(w_.real()) != 0 && Z.w());

        alpha_ -= Z.alpha() + shift;
        beta_ -= Z.beta() + shift;

        return *this;
    }

    /*
     * TODO This isn't working correctly
     */
    // ZOmega& operator*=(const ZOmega& Z)
    //{
    //   using namespace std;
    //   a_ = a_*Z.d() + b_*Z.c() + c_*Z.b() + d_*Z.a();
    //   b_ = -a_*Z.a() + b_*Z.d() + c_*Z.c() + d_*Z.b();
    //   c_ = -a_*Z.b() - b_*Z.a() + c_*Z.d() + d_*Z.c();
    //   d_ = -a_*Z.c() - b_*Z.b() - c_*Z.a() + d_*Z.d();
    //
    //   ZSqrt2 shift(0, round(w_.real()) != 0 && Z.w());

    //  alpha_ *= Z.alpha() + shift;
    //  beta_ *= Z.beta() + shift;

    //  decimal_ = alpha_.decimal() + beta_.decimal()*Im + w_*OMEGA;
    //
    //  return *this;
    //}

    /*
     * Prints the decimal value along with the standard representation,
     *
     *      u = a*OMEGA^3 + b*OMEGA^2 + c*OMEGA + d
     *
     *  with a,b,c,d all integers. The output format is
     *
     *      (a,b,c,d)   decimal_    decimal_dot_
     */
    void print_decimal_standard(const int colw = COLW, const int prec = PREC) {
        std::cout << std::setw(2 * colw) << std::setfill(' ') << std::left
                  << this->get_standard_string() << std::setprecision(prec)
                  << std::setw(2 * colw) << (*this).decimal()
                  << std::setw(2 * colw) << (*this).dot().decimal()
                  << std::endl;
    }

    /*
     * Prints this decimal value along with the in the ZSqrt2 representation,
     *
     *      u = alpha + beta*Im + w*OMEGA
     *
     * where alpha = a_1 + a_2*SQRT2 and beta = b_1 + b_2*SQRT2 are elements of
     * ZSQRT2. The output format is,
     *
     *      (a_1,a_2,b_1,b_2,w)     decimal_    decimal_dot_
     */
    void print_decimal_zsqrt2(const int colw = COLW, const int prec = PREC) {
        std::cout << std::setw(2 * colw) << std::setfill(' ') << std::left
                  << this->get_zsqrt2_string() << std::setprecision(prec)
                  << std::setw(2 * colw) << (*this).decimal()
                  << std::setw(2 * colw) << (*this).dot().decimal()
                  << std::endl;
    }

    str_t csv_str() {
        using namespace std;
        stringstream ss;
        ss << a_ << "," << b_ << "," << c_ << "," << d_;
        return ss.str();
    }

}; // class ZOmega

inline std::ostream& operator<<(std::ostream& os, const ZOmega& Z) {
    os << "(" << Z.a() << "," << Z.b() << "," << Z.c() << "," << Z.d() << ")";
    return os;
}

inline ZOmega operator*(const ZOmega Z, const int_t& x) {
    return ZOmega(Z.a() * x, Z.b() * x, Z.c() * x, Z.d() * x);
}

inline ZOmega operator*(const int_t& x, const ZOmega& Z) {
    return ZOmega(Z.a() * x, Z.b() * x, Z.c() * x, Z.d() * x);
}

inline bool operator==(const ZOmega& Y, const ZOmega& Z) {
    return (Y.a() == Z.a()) && (Y.b() == Z.b()) && (Y.c() == Z.c()) &&
           (Y.d() == Z.d());
}

inline bool operator!=(const ZOmega& Y, const ZOmega& Z) { return !(Y == Z); }

/*
 * Implements euclidean division on ZOmega
 */
inline ZOmega operator/(const ZOmega& Y, const ZOmega& Z) {
    using namespace std;
    ZOmega n = (Y * Z.conj()) * ((Z * Z.conj()).dot());
    real_t mag = Z.norm().norm();
    return ZOmega(
        int_t(floor(real_t(n.a()) / mag)), int_t(floor(real_t(n.b()) / mag)),
        int_t(floor(real_t(n.c()) / mag)), int_t(floor(real_t(n.d()) / mag)));
}

inline ZOmega operator%(const ZOmega& Y, const ZOmega& Z) {
    // return Y - (Y/Z)*Z;
    ZOmega n = (Y * Z.conj()) * ((Z * Z.conj()).dot());
    int_t k = Z.norm().norm();
    real_t a1 = floor(real_t(n.a() + int_t(k / 2)) / k);
    real_t a2 = floor(real_t(n.b() + int_t(k / 2)) / k);
    real_t a3 = floor(real_t(n.c() + int_t(k / 2)) / k);
    real_t a4 = floor(real_t(n.d() + int_t(k / 2)) / k);

    ZOmega q((int_t(a1)), (int_t(a2)), (int_t(a3)), (int_t(a4)));

    return q * Z - Y;
}

// Containers for rings
using zsqrt2_vec_t = std::vector<ZSqrt2>;
using zsqrt2_pair_t = std::array<ZSqrt2, 2>;
using zomega_vec_t = std::vector<ZOmega>;
using zomega_pair_t = std::array<ZOmega, 2>;

// Constants using rings
const ZSqrt2 LAMBDA(1, 1);
const ZSqrt2 LAMBDA_INV(-1, 1);

const std::array<ZOmega, 8> w_pow_arr{
    {ZOmega(0, 0, 0, 1), ZOmega(0, 0, 1, 0), ZOmega(0, 1, 0, 0),
     ZOmega(1, 0, 0, 0), ZOmega(0, 0, 0, -1), ZOmega(0, 0, -1, 0),
     ZOmega(0, -1, 0, 0), ZOmega(-1, 0, 0, 0)}};

inline ZOmega w_pow(const int l) { return w_pow_arr[(8 + l) % 8]; }

} // namespace grid_synth
} // namespace staq

#endif // RINGS_HPP
