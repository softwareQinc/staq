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
#include "mapping/device.hpp"

#include "mapping/layout/basic.hpp"
#include "mapping/layout/eager.hpp"
#include "mapping/layout/bestfit.hpp"

using namespace staq;

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

// Basic tests for layout generation
/******************************************************************************/
TEST(Layout, Basic) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg orig[9];\n" \
    "CX orig[5],orig[7];\n" \
    "CX orig[7],orig[3];\n" \
    "CX orig[4],orig[0];\n" \
    "CX orig[2],orig[1];\n" \
    "CX orig[6],orig[8];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[5],q[7];\n" \
    "CX q[7],q[3];\n" \
    "CX q[4],q[0];\n" \
    "CX q[2],q[1];\n" \
    "CX q[6],q[8];\n";

  auto program = parser::parse_string(pre, "layout_basic.qasm");
  auto layout = mapping::compute_basic_layout(test_device, *program);
  mapping::apply_layout(layout, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Layout, Eager) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg orig[9];\n" \
    "CX orig[5],orig[7];\n" \
    "CX orig[7],orig[3];\n" \
    "CX orig[4],orig[0];\n" \
    "CX orig[2],orig[1];\n" \
    "CX orig[6],orig[8];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[0],q[1];\n" \
    "CX q[1],q[4];\n" \
    "CX q[7],q[6];\n" \
    "CX q[2],q[3];\n" \
    "CX q[5],q[8];\n";

  auto program = parser::parse_string(pre, "layout_eager.qasm");
  auto layout = mapping::compute_eager_layout(test_device, *program);
  mapping::apply_layout(layout, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Layout, Best_Fit) {
  std::string pre =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg orig[9];\n" \
    "CX orig[2],orig[1];\n" \
    "CX orig[2],orig[1];\n" \
    "CX orig[6],orig[8];\n" \
    "CX orig[7],orig[3];\n" \
    "CX orig[7],orig[3];\n" \
    "CX orig[7],orig[3];\n" \
    "CX orig[5],orig[7];\n" \
    "CX orig[5],orig[7];\n" \
    "CX orig[5],orig[7];\n" \
    "CX orig[5],orig[7];\n" \
    "CX orig[4],orig[0];\n";

  std::string post =
    "OPENQASM 2.0;\n" \
    "\n" \
    "qreg q[9];\n" \
    "CX q[7],q[6];\n" \
    "CX q[7],q[6];\n" \
    "CX q[5],q[8];\n" \
    "CX q[1],q[4];\n" \
    "CX q[1],q[4];\n" \
    "CX q[1],q[4];\n" \
    "CX q[0],q[1];\n" \
    "CX q[0],q[1];\n" \
    "CX q[0],q[1];\n" \
    "CX q[0],q[1];\n" \
    "CX q[2],q[3];\n";

  auto program = parser::parse_string(pre, "layout_best_fit.qasm");
  auto layout = mapping::compute_bestfit_layout(test_device, *program);
  mapping::apply_layout(layout, *program);
  std::stringstream ss;
  ss << *program;

  EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
