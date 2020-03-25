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
#include "mapping/device.hpp"
#include "synthesis/cnot_dihedral.hpp"
#include "utils/templates.hpp"
#include "ast/expr.hpp"

using namespace staq;
using namespace utils;
using namespace ast;

// Testing linear reversible (cnot) synthesis

std::ostream& operator<<(std::ostream& os, const synthesis::cx_dihedral& gate) {
    std::visit(overloaded{[&os, &gate](const std::pair<int, int>& cx) {
                              os << "cnot(" << cx.first << "," << cx.second
                                 << ")";
                          },
                          [&os, &gate](const std::pair<ptr<Expr>, int>& rz) {
                              os << "rz(" << *(rz.first) << "," << rz.second
                                 << ")";
                          }},
               gate);
    return os;
}

std::pair<int, int> cnot(int c, int t) { return std::make_pair(c, t); }
std::pair<ptr<Expr>, int> rz(Angle theta, int t) {
    return std::make_pair(angle_to_expr(theta), t);
}
synthesis::phase_term phase(std::vector<bool> b, Angle theta) {
    return std::make_pair(b, angle_to_expr(theta));
}

// Custom equality to deal with ptr<Expr> in cx_dihedral circuits
bool eq(const synthesis::cx_dihedral& a, const synthesis::cx_dihedral& b) {
    if (a.index() != b.index())
        return false;

    if (std::holds_alternative<std::pair<int, int>>(a)) {
        auto& [c1, t1] = std::get<std::pair<int, int>>(a);
        auto& [c2, t2] = std::get<std::pair<int, int>>(b);

        return ((c1 == c2) && (t1 == t2));
    } else {
        auto& [e1, t1] = std::get<std::pair<ptr<Expr>, int>>(a);
        auto& [e2, t2] = std::get<std::pair<ptr<Expr>, int>>(b);

        return ((e1->constant_eval() == e2->constant_eval()) && (t1 == t2));
    }
}

bool eq(const std::list<synthesis::cx_dihedral>& a,
        const std::list<synthesis::cx_dihedral>& b) {
    if (a.size() != b.size())
        return false;

    bool same = true;
    for (auto i = a.begin(), j = b.begin(); same && i != a.end(); i++, j++)
        same &= eq(*i, *j);

    return same;
}

/******************************************************************************/
TEST(Gray_Synth, Base) {
    std::list<synthesis::phase_term> f;
    f.emplace_back(phase({true, true}, angles::pi_quarter));

    synthesis::linear_op<bool> mat{
        {1, 0},
        {0, 1},
    };
    std::list<synthesis::cx_dihedral> output;

    output.emplace_back(cnot(1, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(1, 0));

    EXPECT_TRUE(eq(synthesis::gray_synth(f, mat), output));
}
/******************************************************************************/

/******************************************************************************/
TEST(Gray_Synth, Toffoli) {
    std::list<synthesis::phase_term> f;
    f.emplace_back(phase({true, false, false}, angles::pi_quarter));
    f.emplace_back(phase({false, true, false}, angles::pi_quarter));
    f.emplace_back(phase({true, true, false}, -angles::pi_quarter));
    f.emplace_back(phase({false, false, true}, angles::pi_quarter));
    f.emplace_back(phase({true, false, true}, -angles::pi_quarter));
    f.emplace_back(phase({false, true, true}, -angles::pi_quarter));
    f.emplace_back(phase({true, true, true}, angles::pi_quarter));

    synthesis::linear_op<bool> mat{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    std::list<synthesis::cx_dihedral> output;

    output.emplace_back(rz(angles::pi_quarter, 2));

    output.emplace_back(rz(angles::pi_quarter, 1));
    output.emplace_back(cnot(2, 1));
    output.emplace_back(rz(-angles::pi_quarter, 1));

    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(2, 0));
    output.emplace_back(rz(-angles::pi_quarter, 0));
    output.emplace_back(cnot(1, 0));
    output.emplace_back(rz(-angles::pi_quarter, 0));
    output.emplace_back(cnot(2, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));

    output.emplace_back(cnot(2, 1));
    output.emplace_back(cnot(2, 0));
    output.emplace_back(cnot(1, 0));

    EXPECT_TRUE(eq(synthesis::gray_synth(f, mat), output));
}
/******************************************************************************/

/******************************************************************************/
TEST(Gray_Synth, Gray_code) {
    std::list<synthesis::phase_term> f;
    f.emplace_back(phase({true, false, false, false}, angles::pi_quarter));
    f.emplace_back(phase({true, true, false, false}, angles::pi_quarter));
    f.emplace_back(phase({true, false, true, false}, angles::pi_quarter));
    f.emplace_back(phase({true, true, true, false}, angles::pi_quarter));
    f.emplace_back(phase({true, false, false, true}, angles::pi_quarter));
    f.emplace_back(phase({true, true, false, true}, angles::pi_quarter));
    f.emplace_back(phase({true, false, true, true}, angles::pi_quarter));
    f.emplace_back(phase({true, true, true, true}, angles::pi_quarter));

    synthesis::linear_op<bool> mat{
        {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    std::list<synthesis::cx_dihedral> output;

    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(3, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(2, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(3, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(1, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(3, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(2, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(3, 0));
    output.emplace_back(rz(angles::pi_quarter, 0));
    output.emplace_back(cnot(1, 0));

    EXPECT_TRUE(eq(synthesis::gray_synth(f, mat), output));
}
/******************************************************************************/

/******************************************************************************/
// This test should mimic the Steiner_Gauss base case
TEST(Gray_Steiner, Base) {
    mapping::Device test_device("Test device", 9,
                                {
                                    {0, 1, 0, 0, 0, 1, 0, 0, 0},
                                    {1, 0, 1, 0, 1, 0, 0, 0, 0},
                                    {0, 1, 0, 1, 0, 0, 0, 0, 0},
                                    {0, 0, 1, 0, 1, 0, 0, 0, 1},
                                    {0, 1, 0, 1, 0, 1, 0, 1, 0},
                                    {1, 0, 0, 0, 1, 0, 1, 0, 0},
                                    {0, 0, 0, 0, 0, 1, 0, 1, 0},
                                    {0, 0, 0, 0, 1, 0, 1, 0, 1},
                                    {0, 0, 0, 1, 0, 0, 0, 1, 0},
                                },
                                {1, 1, 1, 1, 1, 1, 1, 1, 1},
                                {
                                    {0, 0.9, 0, 0, 0, 0.1, 0, 0, 0},
                                    {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
                                    {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
                                    {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
                                    {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
                                    {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
                                    {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
                                    {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.1},
                                    {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0},
                                });

    std::list<synthesis::phase_term> f;
    f.emplace_back(phase({true, true, false, false, true, false, false, false},
                         angles::pi));

    synthesis::linear_op<bool> mat{
        {1, 1, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 1},
    };
    std::list<synthesis::cx_dihedral> output;

    output.emplace_back(cnot(4, 1));
    output.emplace_back(cnot(1, 0));
    output.emplace_back(rz(angles::pi, 0));

    EXPECT_TRUE(eq(synthesis::gray_steiner(f, mat, test_device), output));
}
/******************************************************************************/

/******************************************************************************/
// This test should mimic the Steiner_gauss fill_flush case
TEST(Gray_Steiner, Fill_flush) {
    mapping::Device test_device("Test device", 9,
                                {
                                    {0, 1, 0, 0, 0, 1, 0, 0, 0},
                                    {1, 0, 1, 0, 1, 0, 0, 0, 0},
                                    {0, 1, 0, 1, 0, 0, 0, 0, 0},
                                    {0, 0, 1, 0, 1, 0, 0, 0, 1},
                                    {0, 1, 0, 1, 0, 1, 0, 1, 0},
                                    {1, 0, 0, 0, 1, 0, 1, 0, 0},
                                    {0, 0, 0, 0, 0, 1, 0, 1, 0},
                                    {0, 0, 0, 0, 1, 0, 1, 0, 1},
                                    {0, 0, 0, 1, 0, 0, 0, 1, 0},
                                },
                                {1, 1, 1, 1, 1, 1, 1, 1, 1},
                                {
                                    {0, 0.9, 0, 0, 0, 0.1, 0, 0, 0},
                                    {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
                                    {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
                                    {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
                                    {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
                                    {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
                                    {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
                                    {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.1},
                                    {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0},
                                });

    std::list<synthesis::phase_term> f;
    f.emplace_back(
        phase({true, false, true, false, false, false, true, false, false},
              angles::pi));

    synthesis::linear_op<bool> mat{
        {1, 0, 1, 0, 0, 0, 1, 0}, {0, 1, 1, 0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 1, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 1, 1},
    };
    std::list<synthesis::cx_dihedral> output;

    output.emplace_back(cnot(1, 0));
    output.emplace_back(cnot(4, 1));
    output.emplace_back(cnot(7, 4));
    output.emplace_back(cnot(2, 1));
    output.emplace_back(cnot(6, 7));
    output.emplace_back(cnot(7, 4));
    output.emplace_back(cnot(4, 1));
    output.emplace_back(cnot(1, 0));
    output.emplace_back(rz(angles::pi, 0));

    EXPECT_TRUE(eq(synthesis::gray_steiner(f, mat, test_device), output));
}
/******************************************************************************/
