#ifndef REGIONS_HPP
#define REGIONS_HPP

#include <array>
#include <cmath>
#include <gmpxx.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "constants.hpp"
#include "grid_operators.hpp"
#include "rings.hpp"
#include "utils.hpp"
#include "gmp_functions.hpp"

namespace staq {
namespace grid_synth {

/*
 *  Interval type must have +,-,/,*,+=,-=,*=,/=, and = implemented.
 */
template <typename bound_t>
class Interval {

  private:
    bound_t lo_;
    bound_t hi_;
    bound_t width_;

  public:
    Interval(const bound_t& lo, const bound_t& hi)
        : lo_(lo), hi_(hi), width_(hi - lo) {
        try {
            if (lo > hi)
                throw std::invalid_argument(
                    "Interval constructor expects lo < hi, found lo > hi.");
        } catch (std::invalid_argument const& ex) {
            std::cout << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    bound_t lo() const { return lo_; }

    bound_t hi() const { return hi_; }

    bound_t width() const { return width_; }

    void rescale(const bound_t& scale_factor) {
        if (scale_factor < 0) {
            lo_ = hi_ * scale_factor;
            hi_ = lo_ * scale_factor;
            width_ = hi_ - lo_;
        }

        hi_ = hi_ * scale_factor;
        lo_ = lo_ * scale_factor;
        width_ = hi_ - lo_;
    }

    void shift(const bound_t& shift_factor) {
        lo_ = lo_ + shift_factor;
        hi_ = hi_ + shift_factor;
    }

    bool contains(const bound_t& x, const real_t tol=TOL) const { 
      return ((hi_ - x) * (x - lo_) > 0) or (abs((hi_-x)*(x-lo_)) < tol); 
    }

    Interval operator+(const bound_t& shift_factor) const {
        return Interval<bound_t>(lo_ + shift_factor, hi_ + shift_factor);
    }

    Interval operator-(const bound_t& shift_factor) const {
        return Interval<bound_t>(lo_ - shift_factor, hi_ - shift_factor);
    }

    Interval operator*(const bound_t& scale_factor) const {
        if (scale_factor < 0)
            return Interval(hi_ * scale_factor, lo_ * scale_factor);
        return Interval<bound_t>(lo_ * scale_factor, hi_ * scale_factor);
    }

    Interval operator/(const bound_t& scale_factor) const {
        if (scale_factor < 0)
            return Interval(hi_ / scale_factor, lo_ / scale_factor);
        return Interval<bound_t>(lo_ / scale_factor, hi_ / scale_factor);
    }

    Interval& operator+=(const bound_t& shift_factor) {
        lo_ += shift_factor;
        hi_ += shift_factor;

        return *this;
    }

    Interval& operator-=(const bound_t& shift_factor) {
        lo_ -= shift_factor;
        hi_ -= shift_factor;

        return *this;
    }

    Interval& operator*=(const bound_t& scale_factor) {
        if (scale_factor < 0) {
            lo_ = hi_ * scale_factor;
            hi_ = lo_ * scale_factor;

            return *this;
        }

        lo_ = lo_ * scale_factor;
        hi_ = hi_ * scale_factor;

        return *this;
    }

    Interval& operator/=(const bound_t& scale_factor) {
        if (scale_factor < 0) {
            lo_ = hi_ * scale_factor;
            hi_ = lo_ * scale_factor;
        }

        lo_ = lo_ * scale_factor;
        hi_ = hi_ * scale_factor;

        return *this;
    }
}; // class Interval

template <typename bound_t>
inline std::ostream& operator<<(std::ostream& os, const Interval<bound_t>& I) {
    os << "[" << I.lo() << "," << I.hi() << "]";
    return os;
}

template <typename bound_t>
class UprightRectangle {
  private:
    Interval<bound_t> x_interval_;
    Interval<bound_t> y_interval_;

    bound_t area_;

  public:
    UprightRectangle(const Interval<bound_t>& x_interval,
                     const Interval<bound_t>& y_interval)
        : x_interval_(x_interval), y_interval_(y_interval),
          area_(x_interval_.width() * y_interval_.width()) {}

    UprightRectangle(const bound_t& xlo, const bound_t& xhi, const bound_t& ylo,
                     const bound_t& yhi)
        : x_interval_(Interval<bound_t>(xlo, xhi)),
          y_interval_(Interval<bound_t>(ylo, yhi)),
          area_(x_interval_.width() * y_interval_.width()) {}

    Interval<bound_t> x_interval() const { return x_interval_; }
    Interval<bound_t> y_interval() const { return y_interval_; }

    bound_t area() const { return area_; }

    void rescale(const bound_t& x_scale_factor, const bound_t& y_scale_factor) {
        x_interval_.rescale(x_scale_factor);
        y_interval_.rescale(y_scale_factor);
    }

    void shift(const bound_t& x_shift_factor, const bound_t& y_shift_factor) {
        x_interval_.shift(x_shift_factor);
        y_interval_.shift(y_shift_factor);
    }

    bool contains(const bound_t& x, const bound_t& y) {
        return (x_interval_.contains(x) && y_interval_.contains(y));
    }

    /*
     *  Treats the complex number z = a + b*Im as a number in R^2 with
     * components (a,b).
     */
    bool contains(const cplx_t& z) {
        return (x_interval_.contains(z.real()) &&
                y_interval_.contains(z.imag()));
    }

}; // class UprightRectangle

template <typename bound_t>
inline std::ostream& operator<<(std::ostream& os,
                                const UprightRectangle<bound_t>& R) {
    os << "[" << R.x_interval().lo() << "," << R.x_interval().hi() << "]"
       << " X "
       << "[" << R.y_interval().lo() << "," << R.y_interval().hi() << "]";
    return os;
}

class Ellipse {
  private:
    using triple_t = std::array<real_t, 3>;

    vec_t center_;
    mat_t D_;

    real_t semi_major_axis_;
    real_t semi_minor_axis_;

    real_t angle_;

    real_t z_;
    real_t e_;

    void get_z_and_e_() {
        z_ = 0.5 * log(D_(1, 1) / D_(0, 0)) / log(LAMBDA.decimal());
        e_ = sqrt(D_(1, 1) * D_(0, 0));
    }

    mat_t get_mat_from_axes_(const real_t& semi_major_axis,
                             const real_t& semi_minor_axis,
                             const real_t& angle) {
        real_t ct = cos(angle.get_d());
        real_t st = sin(angle.get_d());
        // real_t inva = 1/semi_major_axis;
        // real_t invb = 1/semi_minor_axis;
        real_t inva = 1 / semi_minor_axis;
        real_t invb = 1 / semi_major_axis;

        mat_t M{ct * ct * inva * inva + st * st * invb * invb,
                ct * st * (inva * inva - invb * invb),
                ct * st * (inva * inva - invb * invb),
                st * st * inva * inva + ct * ct * invb * invb};

        return M;
    }

    triple_t get_axes_from_mat_(const vec_t& center, const mat_t& D) {
        real_t m = 1 / sqrt(D.determinant());
        real_t msq = m * m;
        real_t T = D.trace();
        real_t shift =
            PI * (0.25 * (sgn<real_t>(center(0)) - sgn<real_t>(center(1))) + 1);
        if ((sgn<real_t>(center(0)) == 0) && (sgn<real_t>(center(1)) == 0))
            shift = 0;

        if ((sgn<real_t>(center(0)) == 1) && (sgn<real_t>(center(1)) == 1))
            shift = 0;

        real_t a1 = sqrt((T * msq + sqrt(T * T * msq * msq - 4 * msq)) / 2);
        real_t a2 = sqrt((T * msq - sqrt(T * T * msq * msq - 4 * msq)) / 2);

        double a1d = a1.get_d();
        double a2d = a2.get_d();
        double odd = D(0, 1).get_d();

        real_t angle;
        if (((2 * odd * a1d * a1d * a2d * a2d) < 1e-15) and
            ((a2d * a2d - a1d * a1d) < 1e-15))
            angle = 0;
        else
            angle = (0.5 * asin((2 * odd * a1d * a1d * a2d * a2d) /
                                (a2d * a2d - a1d * a1d))) +
                    shift;

        return triple_t{{std::max(a1, a2), std::min(a1, a2), angle}};
    }

  public:
    /*
     * Constructs the ellipse centered at center and defined by the matrix D.
     * There is an issue in using this function in general since there is an
     * ambiguity between the ordering of the axes and the angle of the function.
     */
    Ellipse(const vec_t& center, const mat_t& D) : center_(center), D_(D) {
        triple_t axes_angle = get_axes_from_mat_(center_, D_);
        semi_major_axis_ = axes_angle[0];
        semi_minor_axis_ = axes_angle[1];
        angle_ = axes_angle[2];

        get_z_and_e_();
    }

    /*
     * Constructs the ellipse centred at (x0,y0) tilted at an angle angle. The
     * semi_major_axis is the semi axis that would be aligned with x if angle
     * were rotated to be zero.
     */
    Ellipse(const real_t& x0, const real_t& y0, const real_t& semi_major_axis,
            const real_t& semi_minor_axis, const real_t& angle)
        : center_(vec_t{x0, y0}), semi_major_axis_(semi_major_axis),
          semi_minor_axis_(semi_minor_axis), angle_(angle) {
        center_ = {x0, y0};
        D_ = get_mat_from_axes_(semi_major_axis_, semi_minor_axis_, angle_);
        get_z_and_e_();
    }

    /*
     * Constructs the optimal bounding ellipse for the epsilon region at angle
     * angle.
     */
    Ellipse(const real_t& angle, const real_t& eps) : angle_(angle) {
        real_t r0 = (3 - eps * eps) / 3;
        center_ = vec_t{r0 * cos(angle.get_d()), r0 * sin(angle.get_d())};

        semi_major_axis_ = (2 / sqrt(3)) * eps * sqrt(1 - (eps * eps / 4));
        semi_minor_axis_ = (eps * eps / 3);
        D_ = get_mat_from_axes_(semi_major_axis_, semi_minor_axis_, angle_);

        get_z_and_e_();
    }

    mat_t D() const { return D_; }
    real_t D(int i, int j) const { return D_(i, j); }

    vec_t center() const { return center_; }
    real_t center(int i) const { return center_(i); }

    real_t semi_major_axis() const { return semi_major_axis_; }
    real_t semi_minor_axis() const { return semi_minor_axis_; }
    real_t angle() const { return angle_; }

    real_t e() const { return e_; }
    real_t z() const { return z_; }

    real_t determinant() const { return D_.determinant(); }
    real_t area() const { return PI * semi_major_axis_ * semi_minor_axis_; }

    // uprightness of ellipse
    real_t up() const {
        return (PI / 4) * sqrt(D_.determinant() / (D_(0, 0) * D_(1, 1)));
    }

    void rescale(const real_t scale) {
        D_ = (1.0 / (scale * scale)) * D_;
        semi_minor_axis_ = semi_minor_axis_ * abs(scale);
        semi_major_axis_ = semi_major_axis_ * abs(scale);
        center_ = scale * center_;

        get_z_and_e_();
    }

    /*
     *  Normalizes the ellipse so that it's area is Pi, returns the
     * normalization factor.
     */
    real_t normalize() {
        real_t scale = sqrt(sqrt(D_.determinant()));
        rescale(scale);
        return scale;
    }

    bool contains(const vec_t& point, const real_t& tol=TOL) const {
        using namespace std;
        real_t x = (point - center_).transpose() * D_ * (point - center_);
        return (x < real_t(1)) or (abs(x-real_t(1)) < tol);
    }

    bool contains(const real_t& x, const real_t& y, const real_t& tol=TOL) const {
        return contains(vec_t{x, y},tol);
    }

    bool contains(const cplx_t& z, const real_t& tol=TOL) const {
        return contains(vec_t{z.real(), z.imag()},tol);
    }

    UprightRectangle<real_t> bounding_box() const {
        real_t X = sqrt(D_(1, 1) / (D_.determinant()));
        real_t Y = sqrt(D_(0, 0) / (D_.determinant()));

        return UprightRectangle<real_t>(center_(0) - X, center_(0) + X,
                                        center_(1) - Y, center_(1) + Y);
    }

}; // class Ellipse

inline std::ostream& operator<<(std::ostream& os, const Ellipse& A) {
    os << "---" << std::endl;
    os << A.D() << std::endl;
    os << "semi-major axis = " << A.semi_major_axis() << std::endl;
    os << "semi-minor axis = " << A.semi_minor_axis() << std::endl;
    os << "center = "
       << "(" << A.center(0) << "," << A.center(1) << ")" << std::endl;
    os << "---";
    return os;
}

/*
 * Applies the grid operator G to the ellipse A.
 */
inline Ellipse operator*(const SpecialGridOperator& G, const Ellipse& A) {
    return Ellipse(G.inverse().mat_rep() * A.center(),
                   G.transpose().mat_rep() * A.D() * G.mat_rep());
}

inline Ellipse operator*(const mat_t& M, const Ellipse& A) {
    return Ellipse(M.inverse() * A.center(), M.transpose() * A.D() * M);
}

} // namespace grid_synth
} // namespace staq

#endif // REGIONS_HPP
