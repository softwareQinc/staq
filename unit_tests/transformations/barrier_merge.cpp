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
#include "parser/parser.hpp"
#include "transformations/barrier_merge.hpp"

using namespace staq;

// Testing merging of adjacent barriers
/******************************************************************************/
TEST(BarrierMerge, Adjacent) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "barrier q[0];\n"
                      "barrier q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "barrier q[0],q[1];\n";

    auto program = parser::parse_string(pre, "adjacent.qasm");
    transformations::merge_barriers(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

// Testing merging of non-adjacent barriers
/******************************************************************************/
TEST(BarrierMerge, NonAdjacent) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "barrier q[0];\n"
                      "CX q[0],q[1];\n"
                      "barrier q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "barrier q[0];\n"
                       "CX q[0],q[1];\n"
                       "barrier q[1];\n";

    auto program = parser::parse_string(pre, "nonadjacent.qasm");
    transformations::merge_barriers(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
