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
#include "mapping/device.hpp"

#include "mapping/mapping/swap.hpp"
#include "mapping/mapping/steiner.hpp"

using namespace synthewareQ;

static mapping::Device test_device(
	"Test device",
	9,
	{ {0, 1, 0, 0, 0, 1, 0, 0, 0},
	  {1, 0, 1, 0, 1, 0, 0, 0, 0},
	  {0, 1, 0, 1, 0, 0, 0, 0, 0},
	  {0, 0, 1, 0, 1, 0, 0, 0, 1},
	  {0, 1, 0, 1, 0, 1, 0, 1, 0},
	  {1, 0, 0, 0, 1, 0, 1, 0, 0},
	  {0, 0, 0, 0, 0, 1, 0, 1, 0},
	  {0, 0, 0, 0, 1, 0, 1, 0, 1},
	  {0, 0, 0, 1, 0, 0, 0, 1, 0}, },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ {0, 0.9, 0, 0, 0, 0.1, 0, 0, 0},
	  {0.1, 0, 0.1, 0, 0.8, 0, 0, 0, 0},
	  {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
	  {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
	  {0, 0.1, 0, 0.1, 0, 0.1, 0, 0.7, 0},
	  {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
	  {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
	  {0, 0, 0, 0, 0.1, 0, 0.6, 0, 0.5},
	  {0, 0, 0, 0.1, 0, 0, 0, 0.1, 0}, }
);

// Basic tests for mapping CNOT gates
/******************************************************************************/
TEST(Swap_Mapper, Base) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[0],q[2];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[0],q[1];\n" \
    "CX q[1],q[0];\n" \
    "CX q[0],q[1];\n" \
    "CX q[1],q[2];\n";

  auto program = parser::parse_string(pre, "swap_base.qasm");
  mapping::map_onto_device(test_device, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Swap_Mapper, Shortest_Path) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[0],q[6];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[0],q[1];\n" \
    "CX q[1],q[0];\n" \
    "CX q[0],q[1];\n" \
    "CX q[1],q[4];\n" \
    "CX q[4],q[1];\n" \
    "CX q[1],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[7],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[7],q[6];\n";

  auto program = parser::parse_string(pre, "swap_shortest_path.qasm");
  mapping::map_onto_device(test_device, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Mapper, Base) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[0],q[2];\n" \
    "CX q[0],q[6];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[1],q[2];\n" \
    "CX q[1],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[7],q[6];\n" \
    "CX q[4],q[7];\n" \
    "CX q[1],q[4];\n" \
    "CX q[0],q[1];\n" \
    "CX q[1],q[2];\n" \
    "CX q[1],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[7],q[6];\n" \
    "CX q[4],q[7];\n" \
    "CX q[1],q[4];\n" \
    "CX q[0],q[1];\n";

  auto program = parser::parse_string(pre, "steiner_base.qasm");
  mapping::steiner_mapping(test_device, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Mapper, Swap) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[7],q[1];\n" \
    "CX q[1],q[7];\n" \
    "CX q[7],q[1];\n" \
    "U(0,0,pi/4) q[1];\n" \
    "CX q[7],q[1];\n" \
    "CX q[1],q[7];\n" \
    "CX q[7],q[1];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "U(0,0,0.785398) q[7];\n";

  auto program = parser::parse_string(pre, "steiner_swap.qasm");
  mapping::steiner_mapping(test_device, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Mapper, Swap_No_Z) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[7],q[1];\n" \
    "CX q[1],q[7];\n" \
    "CX q[7],q[1];\n" \
    "U(0,pi/4,0) q[1];\n" \
    "CX q[7],q[1];\n" \
    "CX q[1],q[7];\n" \
    "CX q[7],q[1];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[7],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[4],q[1];\n" \
    "CX q[7],q[4];\n" \
    "CX q[1],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[4],q[1];\n" \
    "CX q[7],q[4];\n" \
    "U(0,pi/4,0) q[1];\n" \
    "CX q[7],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[4],q[1];\n" \
    "CX q[7],q[4];\n" \
    "CX q[1],q[4];\n" \
    "CX q[4],q[7];\n" \
    "CX q[4],q[1];\n" \
    "CX q[7],q[4];\n";

  auto program = parser::parse_string(pre, "steiner_swap_no_z.qasm");
  mapping::steiner_mapping(test_device, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
