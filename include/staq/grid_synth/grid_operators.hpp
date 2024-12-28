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

#ifndef GRID_SYNTH_GRID_OPERATORS_H_
#define GRID_SYNTH_GRID_OPERATORS_H_

#define MATOUTPUT_WIDTH 10

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "staq/grid_synth/constants.hpp"
#include "staq/grid_synth/rings.hpp"
#include "staq/grid_synth/types.hpp"

namespace staq {
namespace grid_synth {

/*
 * Implements a grid operator G,
 *
 *      | a + a'/SQRT2    b + b'/SQRT2 |
 *  G = |                              |
 *      | c + c'/SQRT2    d + d'/SQRT2 |
 *
 * with a + b + c + d ~ 0 mod(2) and a ~ b ~ c ~ d mod(2).
 * G acts on R^2 with the property G(ZOmega) <= ZOmega. Notice that even if the
 * determinant is non-zero we are not guaranteed to have G be invertible.
 */
class GridOperator {
  protected:
    int_t a_;
    int_t ap_;
    int_t b_;
    int_t bp_;
    int_t c_;
    int_t cp_;
    int_t d_;
    int_t dp_;

  public:
    GridOperator(const int_t& a, const int_t& ap, const int_t& b,
                 const int_t& bp, const int_t& c, const int_t& cp,
                 const int_t& d, const int_t& dp)
        : a_(a), ap_(ap), b_(b), bp_(bp), c_(c), cp_(cp), d_(d), dp_(dp) {
        try {
            if (((a_ + b_ + c_ + d_) % 2) != 0) {
                throw std::invalid_argument(
                    "GridOperator expects a + b + c + d ~ 0 mod(2)");
            }

            if ((abs(ap_ % 2) != abs(dp_ % 2)) ||
                (abs(bp_ % 2) != abs(dp_ % 2)) ||
                (abs(cp_ % 2) != abs(dp_ % 2)) ||
                (abs(dp_ % 2) != abs(dp_ % 2))) {
                throw std::invalid_argument(
                    "GridOperator expects a' ~ b' ~ c' ~ d' mod(2)");
            }
        } catch (std::invalid_argument const& ex) {
            std::cout << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    int_t a() const { return a_; }
    int_t ap() const { return ap_; }

    int_t b() const { return b_; }
    int_t bp() const { return bp_; }

    int_t c() const { return c_; }
    int_t cp() const { return cp_; }

    int_t d() const { return d_; }
    int_t dp() const { return dp_; }

    mat_t mat_rep() const {
        return mat_t{a_ + INV_SQRT2 * ap_, b_ + INV_SQRT2 * bp_,
                     c_ + INV_SQRT2 * cp_, d_ + INV_SQRT2 * dp_};
    }

    // Returns sigma*G*sigma
    GridOperator conjugate() const {
        return GridOperator(a_ + ap_, 2 * a_ + ap_, b_, bp_, c_, cp_, -d_ + dp_,
                            2 * d_ - dp_);
    }

    GridOperator inv_conjugate() const {
        return GridOperator(ap_ - a_, 2 * a_ - ap_, b_, bp_, c_, cp_, d_ + dp_,
                            2 * d_ + dp_);
    }

    GridOperator dot() const {
        return GridOperator(a_, -ap_, b_, -bp_, c_, -cp_, d_, -dp_);
    }
    GridOperator transpose() const {
        return GridOperator(a_, ap_, c_, cp_, b_, bp_, d_, dp_);
    }

    real_t determinant() const {
        return (a_ * d_) - (c_ * b_) +
               INV_SQRT2 * ((a_ * dp_) + (d_ * ap_) - (c_ * bp_) - (b_ * cp_)) +
               ((ap_ * dp_) - (cp_ * bp_)) / 2;
    }

}; // class GridOperator

inline std::ostream& operator<<(std::ostream& os, const GridOperator& G) {
    std::stringstream astr, bstr, cstr, dstr;
    astr << "(" << G.a() << "," << G.ap() << ")";
    bstr << "(" << G.b() << "," << G.bp() << ")";
    cstr << "(" << G.c() << "," << G.cp() << ")";
    dstr << "(" << G.d() << "," << G.dp() << ")";

    os << std::setw(MATOUTPUT_WIDTH) << std::left << astr.str()
       << std::setw(MATOUTPUT_WIDTH) << std::left << bstr.str() << std::endl
       << std::setw(MATOUTPUT_WIDTH) << std::left << cstr.str()
       << std::setw(MATOUTPUT_WIDTH) << std::left << dstr.str();
    return os;
}

// Actions of Grid Operator
inline GridOperator operator*(const GridOperator& F, const GridOperator& G) {
    return GridOperator(
        F.a() * G.a() + F.b() * G.c() +
            ((F.ap() * G.ap() + F.bp() * G.cp()) / 2),
        F.a() * G.ap() + F.ap() * G.a() + F.b() * G.cp() + F.bp() * G.c(),
        F.a() * G.b() + F.b() * G.d() +
            ((F.ap() * G.bp() + F.bp() * G.dp()) / 2),
        F.a() * G.bp() + F.ap() * G.b() + F.b() * G.dp() + F.bp() * G.d(),
        F.c() * G.a() + F.d() * G.c() +
            ((F.cp() * G.ap() + F.dp() * G.cp()) / 2),
        F.c() * G.ap() + F.cp() * G.a() + F.d() * G.cp() + F.dp() * G.c(),
        F.c() * G.b() + F.d() * G.d() +
            ((F.cp() * G.bp() + F.dp() * G.dp()) / 2),
        F.c() * G.bp() + F.cp() * G.b() + F.d() * G.dp() + F.dp() * G.d());
}

inline ZOmega operator*(const GridOperator& G, const ZOmega& Z) {
    int_t x = Z.d();
    int_t xp = Z.c() - Z.a();
    int_t y = Z.b();
    int_t yp = Z.c() + Z.a();

    int_t u = x * G.a() + y * G.b() + (yp * G.bp()) / 2 + (xp * G.ap()) / 2;
    int_t up = x * G.ap() + xp * G.a() + y * G.bp() + yp * G.b();
    int_t v = x * G.c() + y * G.d() + (xp * G.cp()) / 2 + (yp * G.dp()) / 2;
    int_t vp = x * G.cp() + xp * G.c() + y * G.dp() + yp * G.d();

    return ZOmega{(vp - up) / 2, v, (vp + up) / 2, u};
}

// Comparison Operators
inline bool operator==(const GridOperator& G, const GridOperator& K) {
    return (G.a() == K.a() && G.ap() == K.ap() && G.b() == K.b() &&
            G.bp() == K.bp() && G.c() == K.c() && G.cp() == K.cp() &&
            G.d() == K.d() && G.dp() == K.dp());
}

inline bool operator!=(const GridOperator& G, const GridOperator& K) {
    return !(G == K);
}

inline GridOperator shift(const GridOperator& G, int_t k) {
    GridOperator newG = G;
    int_t i = 0;
    if (k < 0) {
        while (i < -k) {
            newG = newG.inv_conjugate();
            i++;
        }
        return newG;
    }

    while (i < k) {
        newG = newG.conjugate();
        i++;
    }

    return newG;
}

/*
 * Grid operators with determinant 1. Such grid operators are guaranteed to be
 * invertible and hence this function comes with an inverse method.
 */
class SpecialGridOperator : public GridOperator {
  private:
  public:
    SpecialGridOperator(const GridOperator& G) : GridOperator(G) {
        try {
            if (((a_ * dp_) + (d_ * ap_) - (c_ * bp_) - (b_ * cp_)) != 0) {
                throw std::invalid_argument(
                    "SpecialGridOperator expects ((a_*dp_) + (d_*ap_) - "
                    "(c_*bp_) - (b_*cp_)) == 0");
            }

            if (abs(2 * ((a_ * d_) - (c_ * b_)) + (ap_ * dp_) - (cp_ * bp_)) !=
                2) {
                throw std::invalid_argument(
                    "SpecialGridOperator expects 2*((a_*d_) - (c_*b_)) + "
                    "(ap_*dp_) - (cp_*bp_) == +/- 2");
            }
        } catch (std::invalid_argument const& ex) {
            std::cout << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    SpecialGridOperator(const int_t& a, const int_t& ap, const int_t& b,
                        const int_t& bp, const int_t& c, const int_t& cp,
                        const int_t& d, const int_t& dp)
        : GridOperator(a, ap, b, bp, c, cp, d, dp) {
        try {
            if (((a_ * dp_) + (d_ * ap_) - (c_ * bp_) - (b_ * cp_)) != 0) {
                throw std::invalid_argument(
                    "SpecialGridOperator expects ((a_*dp_) + (d_*ap_) - "
                    "(c_*bp_) - (b_*cp_)) == 0");
            }

            if (abs(2 * ((a_ * d_) - (c_ * b_)) + (ap_ * dp_) - (cp_ * bp_)) !=
                2) {
                throw std::invalid_argument(
                    "SpecialGridOperator expects 2*((a_*d_) - (c_*b_)) + "
                    "(ap_*dp_) - (cp_*bp_) == +/- 2");
            }
        } catch (std::invalid_argument const& ex) {
            std::cout << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    SpecialGridOperator inverse() const {
        int_t det = (a_ * d_) - (c_ * b_) + ((ap_ * dp_) - (cp_ * bp_)) / 2;
        return SpecialGridOperator(d_ * det, dp_ * det, -b_ * det, -bp_ * det,
                                   -c_ * det, -cp_ * det, a_ * det, ap_ * det);
    }

    SpecialGridOperator conjugate() const {
        return SpecialGridOperator(a_ + ap_, 2 * a_ + ap_, b_, bp_, c_, cp_,
                                   -d_ + dp_, 2 * d_ - dp_);
    }

    SpecialGridOperator inv_conjugate() const {
        return SpecialGridOperator(ap_ - a_, 2 * a_ - ap_, b_, bp_, c_, cp_,
                                   d_ + dp_, 2 * d_ + dp_);
    }

    SpecialGridOperator dot() const {
        return SpecialGridOperator(a_, -ap_, b_, -bp_, c_, -cp_, d_, -dp_);
    }

    SpecialGridOperator transpose() const {
        return SpecialGridOperator(a_, ap_, c_, cp_, b_, bp_, d_, dp_);
    }

}; // class SpecialGridOperator

const SpecialGridOperator ID(1, 0, 0, 0, 0, 0, 1, 0);

// Some important grid operators
const SpecialGridOperator R(0, 1, 0, -1, 0, 1, 0, 1);
const SpecialGridOperator K(-1, 1, 0, -1, 1, 1, 0, 1);
const SpecialGridOperator X(0, 0, 1, 0, 1, 0, 0, 0);
const SpecialGridOperator Z(1, 0, 0, 0, 0, 0, -1, 0);

// Grid operators that are raised to some power
inline SpecialGridOperator A(int_t n) {
    return SpecialGridOperator(1, 0, -2 * n, 0, 0, 0, 1, 0);
}

inline SpecialGridOperator B(int_t n) {
    return SpecialGridOperator(1, 0, 0, 2 * n, 0, 0, 1, 0);
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_GRID_OPERATORS_H_
