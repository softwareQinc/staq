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

using namespace staq;
using namespace utils;

// Testing linear reversible (cnot) synthesis

std::ostream& operator<<(std::ostream& os, synthesis::cx_dihedral gate) {
  std::visit(overloaded{
      [&os, &gate](std::pair<int, int> cx){ os << "cnot(" << cx.first << "," << cx.second << ")"; },
      [&os, &gate](std::pair<Angle, int> rz){ os << "rz(" << rz.first << "," << rz.second << ")"; }
    }, gate);
  return os;
}

std::pair<int, int> cnot(int c, int t) { return std::make_pair(c, t); }
std::pair<Angle, int> rz(Angle theta, int t) { return std::make_pair(theta, t); }

/******************************************************************************/
TEST(Gray_Synth, Base) {
  std::list<synthesis::phase_term> f{ { {true, true}, angles::pi_quarter} };
  synthesis::linear_op<bool> mat{ {1, 0}, {0, 1}, };
  std::list<synthesis::cx_dihedral> output;

  output.push_back(cnot(1,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(1,0));
  
  EXPECT_EQ(synthesis::gray_synth(f, mat), output);
}
/******************************************************************************/

/******************************************************************************/
TEST(Gray_Synth, Toffoli) {
  std::list<synthesis::phase_term> f{
    { {true, false, false}, angles::pi_quarter},
    { {false, true, false}, angles::pi_quarter},
    { {true, true, false}, -angles::pi_quarter},
    { {false, false, true}, angles::pi_quarter},
    { {true, false, true}, -angles::pi_quarter},
    { {false, true, true}, -angles::pi_quarter},
    { {true, true, true}, angles::pi_quarter},
  };
  synthesis::linear_op<bool> mat{ {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
  std::list<synthesis::cx_dihedral> output;

  output.push_back(rz(angles::pi_quarter,2));

  output.push_back(rz(angles::pi_quarter,1));
  output.push_back(cnot(2,1));
  output.push_back(rz(-angles::pi_quarter,1));

  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(2,0));
  output.push_back(rz(-angles::pi_quarter,0));
  output.push_back(cnot(1,0));
  output.push_back(rz(-angles::pi_quarter,0));
  output.push_back(cnot(2,0));
  output.push_back(rz(angles::pi_quarter,0));

  output.push_back(cnot(2,1));
  output.push_back(cnot(2,0));
  output.push_back(cnot(1,0));
  
  EXPECT_EQ(synthesis::gray_synth(f, mat), output);
}
/******************************************************************************/

/******************************************************************************/
TEST(Gray_Synth, Gray_code) {
  std::list<synthesis::phase_term> f{
    { {true, false, false, false}, angles::pi_quarter},
    { {true, true, false, false}, angles::pi_quarter},
    { {true, false, true, false}, angles::pi_quarter},
    { {true, true, true, false}, angles::pi_quarter},
    { {true, false, false, true}, angles::pi_quarter},
    { {true, true, false, true}, angles::pi_quarter},
    { {true, false, true, true}, angles::pi_quarter},
    { {true, true, true, true}, angles::pi_quarter},
  };
  synthesis::linear_op<bool> mat{ {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} };
  std::list<synthesis::cx_dihedral> output;

  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(3,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(2,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(3,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(1,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(3,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(2,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(3,0));
  output.push_back(rz(angles::pi_quarter,0));
  output.push_back(cnot(1,0));
  
  EXPECT_EQ(synthesis::gray_synth(f, mat), output);
}
/******************************************************************************/

/******************************************************************************/
// This test should mimic the Steiner_Gauss base case
TEST(Gray_Steiner, Base) {
	mapping::Device test_device(
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
		  {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
		  {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
		  {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
		  {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
		  {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
		  {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
		  {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.1},
		  {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0}, }
	);

  std::list<synthesis::phase_term> f{
    { {true, true, false, false, true, false, false,false}, angles::pi},
  };
  synthesis::linear_op<bool> mat{ {1,1,0,0,1,0,0,0},
                                  {0,1,0,0,1,0,0,0},
                                  {0,0,1,0,0,0,0,0},
                                  {0,0,0,1,0,0,0,0},
                                  {0,0,0,0,1,0,0,0},
                                  {0,0,0,0,0,1,0,0},
                                  {0,0,0,0,0,0,1,0},
                                  {0,0,0,0,0,0,0,1},
  };
  std::list<synthesis::cx_dihedral> output;

  output.push_back(cnot(4,1));
  output.push_back(cnot(1,0));
  output.push_back(rz(angles::pi,0));
  
  EXPECT_EQ(synthesis::gray_steiner(f, mat, test_device), output);
}
/******************************************************************************/

/******************************************************************************/
// This test should mimic the Steiner_gauss fill_flush case
TEST(Gray_Steiner, Fill_flush) {
	mapping::Device test_device(
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
		  {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
		  {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
		  {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
		  {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
		  {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
		  {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
		  {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.1},
		  {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0}, }
	);

  std::list<synthesis::phase_term> f{
    { {true, false, true, false, false, false, true, false, false}, angles::pi},
  };
  synthesis::linear_op<bool> mat{ {1,0,1,0,0,0,1,0},
                                  {0,1,1,0,0,0,1,0},
                                  {0,0,1,0,0,0,0,0},
                                  {0,0,0,1,0,0,0,0},
                                  {0,0,0,0,1,0,1,0},
                                  {0,0,0,0,0,1,0,0},
                                  {0,0,0,0,0,0,1,0},
                                  {0,0,0,0,0,0,1,1},
  };
  std::list<synthesis::cx_dihedral> output;

  output.push_back(cnot(1,0));
  output.push_back(cnot(4,1));
  output.push_back(cnot(7,4));
  output.push_back(cnot(2,1));
  output.push_back(cnot(6,7));
  output.push_back(cnot(7,4));
  output.push_back(cnot(4,1));
  output.push_back(cnot(1,0));
  output.push_back(rz(angles::pi,0));
  
  EXPECT_EQ(synthesis::gray_steiner(f, mat, test_device), output);
}
/******************************************************************************/
