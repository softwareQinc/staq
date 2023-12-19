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

#ifndef GRID_SYNTH_STATES_HPP_
#define GRID_SYNTH_STATES_HPP_

#include <algorithm>
#include <cmath>

#include <gmpxx.h>

#include "staq/grid_synth/gmp_functions.hpp"
#include "staq/grid_synth/grid_operators.hpp"
#include "staq/grid_synth/regions.hpp"
#include "staq/grid_synth/rings.hpp"

namespace staq {
namespace grid_synth {

using state_t = std::array<Ellipse, 2>;

inline state_t operator*(const GridOperator& G, const state_t& state) {
    return state_t{G * state[0], G.dot() * state[1]};
}

inline real_t skew(const state_t state) {
    return state[0].D(0, 1) * state[0].D(0, 1) +
           state[1].D(0, 1) * state[1].D(0, 1);
}

inline real_t bias(const state_t state) { return state[1].z() - state[0].z(); }

inline int_t determine_shift(const state_t state) {
    return int_t(floor((1 - bias(state)) / 2));
}

inline mat_t sigma(int_t k) {
    if (k < 0) {
        return gmpf::pow(SQRT_LAMBDA_INV, -k) *
               mat_t{1, 0, 0, gmpf::pow(LAMBDA.decimal(), -k)};
    }

    return gmpf::pow(SQRT_LAMBDA_INV, k) *
           mat_t{gmpf::pow(LAMBDA.decimal(), k), 0, 0, 1};
}

inline mat_t tau(int_t k) {
    if (k < 0) {
        return gmpf::pow(SQRT_LAMBDA_INV, -k) *
               mat_t{gmpf::pow(LAMBDA.decimal(), -k), 0, 0, gmpf::pow(-1, -k)};
    }

    return gmpf::pow(SQRT_LAMBDA_INV, k) *
           mat_t{1, 0, 0, gmpf::pow(-LAMBDA.decimal(), k)};
}

/*
 *  Act on the state (A,B) with $k$ copies of the shift operators SIGMA and TAU
 *  and the return the shifted state.
 */
inline state_t shift(const state_t& state, const int_t& k) {
    return state_t{sigma(k) * state[0], tau(k) * state[1]};
}

/*
 * Reduces skew(state) by 10% and returns the operator that did it.
 */
inline SpecialGridOperator reduce_skew(state_t& state) {
    real_t initial_skew = skew(state);
    if (initial_skew < 15) {
        return ID;
    }

    int_t k = 0;
    if (abs(bias(state)) > real_t("1")) {
        k = determine_shift(state);
        state = shift(state, k);
    }
    SpecialGridOperator G = ID;

    if ((state[1].z() + state[0].z()) < real_t("0")) {
        G = G * X;
        state = X * state;
        // std::cout << "X" << std::endl;
    }

    if (state[1].D(0, 1) < real_t("0")) {
        G = G * Z;
        state = Z * state;
        // std::cout << "Y" << std::endl;
    }

    real_t z = state[0].z();
    real_t zeta = state[1].z();

    if (static_cast<bool>(gmpf::gmp_geq(state[0].D(0, 1), real_t("0")))) {
        if (gmpf::gmp_geq(z, real_t("-0.8")) &&
            gmpf::gmp_leq(z, real_t("0.8")) &&
            gmpf::gmp_geq(zeta, real_t("-0.8")) &&
            gmpf::gmp_leq(zeta, real_t("0.8"))) {
            G = G * R;
            state = R * state;
            // std::cout << "R" << std::endl;
        } else if (gmpf::gmp_leq(z, real_t("0.3")) &&
                   gmpf::gmp_geq(zeta, real_t("0.8"))) {
            G = G * K;
            state = K * state;
            // std::cout << "K" << std::endl;
        } else if (gmpf::gmp_geq(z, real_t("0.3")) &&
                   gmpf::gmp_geq(zeta, real_t("0.3"))) {
            real_t c = gmpf::gmp_min(z, zeta);
            int_t n = gmpf::gmp_max(
                1, int_t(gmpf::gmp_floor(
                       gmpf::pow(LAMBDA.decimal(), c.get_ui()) / 2)));
            G = G * A(n);
            state = A(n) * state;
            // std::cout << "A^" << n << std::endl;
        } else if (gmpf::gmp_geq(z, real_t("0.8")) &&
                   gmpf::gmp_leq(zeta, real_t("0.3"))) {
            G = G * K.dot();
            state = K.dot() * state;
            // std::cout << "K.dot()" << std::endl;
        } else {
            std::cout << "reduce skew did not find any valid cases with the "
                         "ellipses: "
                      << std::endl
                      << state[0] << std::endl
                      << "======" << std::endl
                      << state[1] << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        if (gmpf::gmp_geq(z, real_t("-0.8")) &&
            gmpf::gmp_leq(z, real_t("0.8")) &&
            gmpf::gmp_geq(zeta, real_t("-0.8")) &&
            gmpf::gmp_leq(zeta, real_t("0.8"))) {
            G = G * R;
            state = R * state;
            // std::cout << "R" << std::endl;
        } else if (gmpf::gmp_geq(z, real_t("-0.2")) &&
                   gmpf::gmp_geq(zeta, real_t("-0.2"))) {
            real_t c = gmpf::gmp_min(z, zeta);
            int_t n = gmpf::gmp_max(
                1, int_t(gmpf::gmp_floor(
                       gmpf::pow(LAMBDA.decimal(), c.get_ui()) / 2)));
            G = G * B(n);
            state = B(n) * state;
            // std::cout << "B^" << n << std::endl;
        } else {
            std::cout << "reduce skew did not find any valid cases with the "
                         "ellipses: "
                      << std::endl
                      << state[0] << std::endl
                      << "======" << std::endl
                      << state[1] << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    state = shift(state, k);

    if (skew(state) > real_t("0.9") * initial_skew) {
        std::cout << "reduce_skew failed to reduce the skew by at least 10%. "
                     "Exiting.";
        exit(EXIT_FAILURE);
    }

    return shift(G, k);
}

/*
 * Accepts a state with arbitrary normalization and returns a state with the
 * original normalization but with the skew reduced to its lowest possible value
 */
inline SpecialGridOperator optimize_skew(state_t& state) {
    real_t scaleA = state[0].normalize();
    real_t scaleB = state[1].normalize();
    SpecialGridOperator G = ID;

    while (skew(state) >= 15) {
        G = G * reduce_skew(state);
    }

    state[0].rescale(real_t("1") / scaleA);
    state[1].rescale(real_t("1") / scaleB);

    return G;
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_STATES_HPP_
