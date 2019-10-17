/*
 * This file is part of synthewareQ.
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

using namespace synthewareQ;

// Testing rotation folding optimization
/******************************************************************************/
TEST(Rotation_folding, TMerge) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "t q[0];\n" \
    "t q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "s q[0];\n";

  auto program = parser::parse_string(pre, "TMerge.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, TCancel) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "t q[0];\n" \
    "tdg q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n";

  auto program = parser::parse_string(pre, "TCancel.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, TNoMerge) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "t q[0];\n" \
    "x q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "t q[0];\n" \
    "x q[0];\n";

  auto program = parser::parse_string(pre, "TNoMerge.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, TConjMerge) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "h q[0];\nt q[0];\nh q[0];\n" \
    "x q[0];\n" \
    "h q[0];\nt q[0];\nh q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "h q[0];\nh q[0];\n" \
    "x q[0];\n" \
    "h q[0];\ns q[0];\nh q[0];\n";

  auto program = parser::parse_string(pre, "TConjMerge.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, RzMerge) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "rz(pi/16) q[0];\n" \
    "rz(pi/16) q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "rz(0.392699) q[0];\n"; // pi/8 = 0.392699

  auto program = parser::parse_string(pre, "RzMerge.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, RxMerge) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "rx(pi/16) q[0];\n" \
    "rx(pi/16) q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "rx(0.392699) q[0];\n"; // pi/8 = 0.392699

  auto program = parser::parse_string(pre, "RxMerge.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, TCNOTMerge) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[2];\n" \
    "cx q[0],q[1];\nt q[1];\ncx q[0],q[1];\n" \
    "cx q[1],q[0];\nt q[0];\ncx q[1],q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[2];\n" \
    "cx q[0],q[1];\ncx q[0],q[1];\n" \
    "cx q[1],q[0];\ns q[0];\ncx q[1],q[0];\n";

  auto program = parser::parse_string(pre, "TCNOTMerge.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Rotation_folding, GlobalPhase) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "t q[0];\n" \
    "x q[0];\n" \
    "t q[0];\n" \
    "x q[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "include \"qelib1.inc\";\n" \
    "\n" \
    "qreg q[1];\n" \
    "x q[0];\n" \
    "h q[0];\ns q[0];\nh q[0];\ns q[0];\nh q[0];\ns q[0];\n" \
    "x q[0];\n";

  auto program = parser::parse_string(pre, "GlobalPhase.qasm");
  optimization::fold_rotations(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
