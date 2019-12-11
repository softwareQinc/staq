/*
 * This file is part of staq.
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

#include "gtest/gtest.h"
#include "utils/angle.hpp"

using namespace staq::utils;

// Tiny unit tests for angles

/******************************************************************************/
TEST(Angles, Pi) { EXPECT_EQ(angles::pi.numeric_value(), pi); }
/******************************************************************************/

/******************************************************************************/
TEST(Angles, Symbolic_Symbolic) {
    EXPECT_TRUE((angles::pi_quarter + angles::pi_half).is_symbolic());
    EXPECT_TRUE((angles::pi_quarter - angles::pi_half).is_symbolic());
    EXPECT_TRUE((angles::pi_quarter * 2).is_symbolic());
    EXPECT_TRUE((angles::pi_quarter / 2).is_symbolic());
}
/******************************************************************************/

/******************************************************************************/
TEST(Angles, Symbolic_Numeric) {
    EXPECT_TRUE((angles::pi_quarter + Angle(1.1)).is_numeric());
    EXPECT_TRUE((Angle(1.1) - angles::pi_quarter).is_numeric());
}
/******************************************************************************/

/******************************************************************************/
TEST(Angles, Arithmetic) {
    EXPECT_EQ(-angles::pi_quarter, Angle(7, 4));
    EXPECT_EQ(-angles::pi_quarter,
              angles::pi_quarter + angles::pi_half + angles::pi);
    EXPECT_EQ(angles::pi_quarter / 2, Angle(1, 8));
    EXPECT_EQ(angles::pi_quarter * 2, angles::pi_half);
}
/******************************************************************************/
