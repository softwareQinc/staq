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

#ifndef GRID_SYNTH_TYPES_HPP_
#define GRID_SYNTH_TYPES_HPP_

#include <complex>
#include <queue>
#include <string>

#include <gmpxx.h>

#include "staq/grid_synth/complex.hpp"
#include "staq/grid_synth/mat_vec_2x2.hpp"

namespace staq {
namespace grid_synth {

using int_t = mpz_class;
using real_t = mpf_class;
using cplx_t = complex<real_t>;
using str_t = std::string;

using vec_t = col_vec2_t<real_t>;
using mat_t = mat2_t<real_t>;

using int_vec_t = std::vector<int_t>;
using int_queue_t = std::queue<int_t>;
} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_TYPES_HPP_
