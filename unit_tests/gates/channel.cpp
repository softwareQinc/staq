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
#include "gates/channel.hpp"

using namespace staq;

// Testing channel gates
using Gates = gates::ChannelRepr<std::string>;

/******************************************************************************/
TEST(Channel_Rep, Pauli_Arithmetic) {
  auto i1 = Gates::Pauli::i("x1");
  auto x1 = Gates::Pauli::x("x1");
  auto z1 = Gates::Pauli::z("x1");
  auto y1 = Gates::Pauli::y("x1");

  EXPECT_NE(x1, i1);
  EXPECT_EQ(x1*x1, i1);
  EXPECT_EQ(z1*z1, i1);
  EXPECT_EQ(y1*y1, i1);
  EXPECT_NE(z1*z1*y1, i1);
}
/******************************************************************************/

/******************************************************************************/
TEST(Channel_Rep, Pauli_Commute) {
  auto x1 = Gates::Pauli::x("x1");
  auto x2 = Gates::Pauli::x("x2");
  auto z1 = Gates::Pauli::z("x1");
  auto z2 = Gates::Pauli::z("x2");

  EXPECT_TRUE(x1.commutes_with(x1));
  EXPECT_FALSE(x1.commutes_with(z1));
  EXPECT_TRUE(x1.commutes_with(z2));
  EXPECT_TRUE((x1*z2).commutes_with(z1*x2));
}
/******************************************************************************/

/******************************************************************************/
TEST(Channel_Rep, Clifford_Arithmetic) {
  auto i1 = Gates::Pauli::i("x1");
  auto x1 = Gates::Pauli::x("x1");
  auto x2 = Gates::Pauli::x("x2");
  auto z1 = Gates::Pauli::z("x1");
  auto z2 = Gates::Pauli::z("x2");
  auto y1 = Gates::Pauli::y("x1");

  auto h1 = Gates::Clifford::h("x1");
  auto s1 = Gates::Clifford::s("x1");
  auto cnot12 = Gates::Clifford::cnot("x1", "x2");

  EXPECT_EQ(h1.conjugate(x1), z1);
  EXPECT_EQ(h1.conjugate(z1), x1);
  EXPECT_EQ(h1.conjugate(y1), -y1);

  EXPECT_EQ((h1*h1).conjugate(x1), x1);
  EXPECT_EQ((h1*h1).conjugate(z1), z1);
  EXPECT_EQ((h1*h1).conjugate(y1), y1);

  EXPECT_EQ(cnot12.conjugate(x1), x1 * x2);
  EXPECT_EQ(cnot12.conjugate(x2), x2);
  EXPECT_EQ(cnot12.conjugate(z1), z1);
  EXPECT_EQ(cnot12.conjugate(z2), z1 * z2);
}
/******************************************************************************/

/******************************************************************************/
TEST(Channel_Rep, Channel_Commute) {
  auto t1 = Gates::Rotation::t("x1");
  auto tdg1 = Gates::Rotation::tdg("x1");
  auto t2 = Gates::Rotation::t("x2");
  auto cnot12 = Gates::Clifford::cnot("x1", "x2");
  auto h1 = Gates::Clifford::h("x1");
  auto s1 = Gates::Clifford::s("x1");
  auto u1 = Gates::Uninterp({ "x1" });

  EXPECT_TRUE(t1.commutes_with(tdg1));
  EXPECT_FALSE(t1.commutes_with(u1));
  EXPECT_TRUE(t2.commutes_with(u1));

  EXPECT_EQ(t1.commute_left(cnot12), t1);
  EXPECT_NE(t1.commute_left(h1), t1);
  EXPECT_EQ(t1.commute_left(s1), t1);
  EXPECT_NE(t2.commute_left(cnot12), t1);
}
/******************************************************************************/

/******************************************************************************/
TEST(Channel_Rep, Gate_Merge) {
  auto id1 = Gates::Rotation::rz(utils::angles::zero, "x1");
  auto t1 = Gates::Rotation::t("x1");
  auto tdg1 = Gates::Rotation::tdg("x1");
  auto t2 = Gates::Rotation::t("x2");
  auto s1 = Gates::Rotation::rz(utils::angles::pi_half, "x1");
  auto rtx1 = Gates::Rotation::rx(utils::angles::pi_half, "x1");

  EXPECT_TRUE(t1.try_merge(t1));
  EXPECT_TRUE(t1.try_merge(tdg1));
  EXPECT_FALSE(t1.try_merge(t2));
  EXPECT_TRUE(t1.try_merge(s1));
  EXPECT_FALSE(t1.try_merge(rtx1));

  EXPECT_EQ(t1.try_merge(t1)->second, s1);
  EXPECT_EQ(t1.try_merge(tdg1)->second, id1);

  auto x1 = Gates::Clifford::x("x1");

  EXPECT_TRUE(t1.try_merge(tdg1.commute_left(x1)));
  EXPECT_EQ(t1.try_merge(tdg1.commute_left(x1))->first, -utils::angles::pi_quarter);
  EXPECT_EQ(t1.try_merge(tdg1.commute_left(x1))->second, s1);
}
/******************************************************************************/
