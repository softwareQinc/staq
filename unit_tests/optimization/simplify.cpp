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
#include "optimization/simplify.hpp"

using namespace staq;

// Testing basic simplifications
/******************************************************************************/
TEST(Simplify, H_Cancel) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "h q[0];\n"
                      "h q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n";

    auto program = parser::parse_string(pre, "h_cancel.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Simplify, S_Cancel) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "sdg q[0];\n"
                      "s q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n";

    auto program = parser::parse_string(pre, "s_cancel.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Simplify, CX_Cancel) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[0],q[1];\n"
                      "cx q[0],q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n";

    auto program = parser::parse_string(pre, "cx_cancel.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Simplify, No_Cancel) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "h q[0];\n"
                      "barrier q[0];\n"
                      "h q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "h q[0];\n"
                       "barrier q[0];\n"
                       "h q[0];\n";

    auto program = parser::parse_string(pre, "no_cancel.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Simplify, Disjoint_Qubits) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "h q[0];\n"
                      "barrier q[1];\n"
                      "h q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "barrier q[1];\n";

    auto program = parser::parse_string(pre, "disjoint_qubits.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Simplify, Serial_Cancellation) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "z q[0];\n"
                      "z q[0];\n"
                      "x q[0];\n"
                      "x q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n";

    auto program = parser::parse_string(pre, "serial_cancellation.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Simplify, Nested_Cancellation) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "x q[0];\n"
                      "z q[0];\n"
                      "z q[0];\n"
                      "x q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n";

    auto program = parser::parse_string(pre, "nested_cancellation.qasm");
    optimization::simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
