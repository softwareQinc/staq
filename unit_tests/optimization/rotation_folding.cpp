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
#include "optimization/rotation_folding.hpp"

using namespace staq;

// Testing rotation folding optimization
/******************************************************************************/
TEST(Rotation_folding, T_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "t q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "s q[0];\n";

    auto program = parser::parse_string(pre, "t_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, T_Cancel) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "tdg q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n";

    auto program = parser::parse_string(pre, "t_cancel.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, T_No_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "x q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "t q[0];\n"
                       "x q[0];\n";

    auto program = parser::parse_string(pre, "t_no_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, T_Conj_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "h q[0];\nt q[0];\nh q[0];\n"
                      "x q[0];\n"
                      "h q[0];\nt q[0];\nh q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "h q[0];\nh q[0];\n"
                       "x q[0];\n"
                       "h q[0];\ns q[0];\nh q[0];\n";

    auto program = parser::parse_string(pre, "t_conj_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, Rz_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "rz(pi/16) q[0];\n"
                      "rz(pi/16) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "rz(0.392699) q[0];\n"; // pi/8 = 0.392699

    auto program = parser::parse_string(pre, "rz_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, Rx_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "rx(pi/16) q[0];\n"
                      "rx(pi/16) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "rx(0.392699) q[0];\n"; // pi/8 = 0.392699

    auto program = parser::parse_string(pre, "rx_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, T_CNOT_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[0],q[1];\nt q[1];\ncx q[0],q[1];\n"
                      "cx q[1],q[0];\nt q[0];\ncx q[1],q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "cx q[0],q[1];\ncx q[0],q[1];\n"
                       "cx q[1],q[0];\ns q[0];\ncx q[1],q[0];\n";

    auto program = parser::parse_string(pre, "t_cnot_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, Global_Phase) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "x q[0];\n"
                      "t q[0];\n"
                      "x q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "x q[0];\n"
                       "h q[0];\ns q[0];\nh q[0];\ns q[0];\nh q[0];\ns q[0];\n"
                       "x q[0];\n";

    auto program = parser::parse_string(pre, "global_phase.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
