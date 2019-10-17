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
#include "transformations/desugar.hpp"

using namespace synthewareQ;

// Testing desugaring of mapped gates
/******************************************************************************/
TEST(Desugar, One_Qubit) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "U(0,0,0) q;\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "U(0,0,0) q[0];\n" \
    "U(0,0,0) q[1];\n";

  auto program = parser::parse_string(pre, "one_qubit.qasm");
  transformations::desugar(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Desugar, Two_Qubit) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "qreg p[2];\n" \
    "CX q,p;\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "qreg p[2];\n" \
    "CX q[0],p[0];\n" \
    "CX q[1],p[1];\n";

  auto program = parser::parse_string(pre, "two_qubit.qasm");
  transformations::desugar(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Desugar, Multi_Qubit) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "qreg p[2];\n" \
    "qreg r[2];\n" \
    "barrier q,p,r;\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "qreg p[2];\n" \
    "qreg r[2];\n" \
    "barrier q[0],p[0],r[0];\n" \
    "barrier q[1],p[1],r[1];\n";

  auto program = parser::parse_string(pre, "multi_qubit.qasm");
  transformations::desugar(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Desugar, Mixin) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "qreg p[2];\n" \
    "qreg r[2];\n" \
    "barrier q,p[1],r;\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[2];\n" \
    "qreg p[2];\n" \
    "qreg r[2];\n" \
    "barrier q[0],p[1],r[0];\n" \
    "barrier q[1],p[1],r[1];\n";

  auto program = parser::parse_string(pre, "mixin.qasm");
  transformations::desugar(*program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
