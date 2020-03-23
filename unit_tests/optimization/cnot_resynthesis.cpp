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
#include "optimization/cnot_resynthesis.hpp"

using namespace staq;

// Testing cnot resynthesis
/******************************************************************************/
TEST(CNOT_resynthesis, Base) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[1],q[0];\n"
                      "t q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "cx q[1],q[0];\n"
                       "t q[0];\n";

    auto program = parser::parse_string(pre, "base.qasm");
    optimization::optimize_CNOT(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(CNOT_resynthesis, Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[1],q[0];\n"
                      "t q[0];\n"
                      "t q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "cx q[1],q[0];\n"
                       "s q[0];\n";

    auto program = parser::parse_string(pre, "merge.qasm");
    optimization::optimize_CNOT(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(CNOT_resynthesis, Decl_opt) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "gate foo a,b,c {\n"
                      "\tt c;\n"
                      "\tcx c,b;\n"
                      "\tcx a,b;\n"
                      "\tt b;\n"
                      "\tcx b,a;\n"
                      "\tt a;\n"
                      "\tcx a,c;\n"
                      "\tcx b,c;\n"
                      "\tt c;\n"
                      "\tcx b,a;\n"
                      "\tcx a,c;\n"
                      "\tcx a,b;\n"
                      "\tcx c,b;\n"
                      "}\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "gate foo a,b,c {\n"
                       "\tt c;\n"
                       "\tcx b,c;\n"
                       "\tt c;\n"
                       "\tcx a,c;\n"
                       "\tt c;\n"
                       "\tcx b,c;\n"
                       "\tt c;\n"
                       "\tcx a,c;\n"
                       "}\n";

    auto program = parser::parse_string(pre, "decl_opt.qasm");
    optimization::optimize_CNOT(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
