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

#ifndef GRID_SYNTH_EXACT_SYNTHESIS_HPP_
#define GRID_SYNTH_EXACT_SYNTHESIS_HPP_

#include <cstdlib>

#include "matrix.hpp"
#include "types.hpp"

namespace staq {
namespace grid_synth {

// Returns known common cases for multiples of theta = pi/4. w =
// 1/(sqrt(omega)).
inline str_t check_common_cases(real_t theta, const real_t& eps) {

    while (theta > real_t("2"))
        theta = theta - real_t("2");
    while (theta < 0)
        theta = theta + real_t("2");

    if (abs(theta - real_t("0.25")) < eps) {
        return "Tw";
    } else if (abs(theta - real_t("0.5")) < eps) {
        return "SWWWWWWW";
    } else if (abs(theta - real_t("0.75")) < eps) {
        return "STWWWWWWWw";
    } else if (abs(theta - real_t("1")) < eps) {
        return "SSWWWWWW";
    } else if (abs(theta - real_t("1.25")) < eps) {
        return "SSTWWWWWWw";
    } else if (abs(theta - real_t("1.5")) < eps) {
        return "SSSWWWWW";
    } else if (abs(theta - real_t("1.75")) < eps) {
        return "SSSTWWWWWw";
    } else if (abs(theta - real_t("2")) < eps) {
        return "WWWW";
    } else {
        return "";
    }
}

inline str_t synthesize(const DOmegaMatrix& D,
                        const domega_matrix_table_t& s3_table) {
    using namespace std;

    int_t s = D.sde_u_sq();
    int_t sold = D.sde_u_sq();
    DOmegaMatrix running_D = D;
    str_t op_str = "";

    while (s > 3) {
        DOmegaMatrix op = H;
        DOmegaMatrix temp_D = op * running_D;
        str_t temp_str = "H";
        if (temp_D.sde_u_sq() == s - 1) {
            op_str += temp_str;
            running_D = temp_D;
            s = temp_D.sde_u_sq();
            continue;
        }

        for (int k = 1; k < 4; k++) {
            op = op * (T.dagger());
            temp_D = op * running_D;
            temp_str = "T" + temp_str;
            if (temp_D.sde_u_sq() == s - 1) {
                op_str += temp_str;
                running_D = temp_D;
                s = temp_D.sde_u_sq();
                break;
            }
        }
        if (sold == s) {
            cout << "Value of s not changed" << endl;
            exit(EXIT_FAILURE);
        }
        sold = sold - 1;
    }
    str_t rem = s3_table.at(running_D);
    return op_str += rem;
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_EXACT_SYNTHESIS_HPP_
