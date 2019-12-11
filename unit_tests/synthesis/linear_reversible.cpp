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
#include "synthesis/linear_reversible.hpp"

using namespace staq;
using circuit = std::list<std::pair<int, int> >;

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
      {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
      {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
      {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
      {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
      {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
      {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
      {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.1},
      {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0}, }
);

// Testing linear reversible (cnot) synthesis

/******************************************************************************/
TEST(Gaussian_Synthesis, Base) {
	synthesis::linear_op<bool> mat{ { 1, 0}, { 1, 1}, };
	EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({ {0,1} }));
	EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({ {0,1} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Gaussian_Synthesis, Swap) {
	synthesis::linear_op<bool> mat{ { 0, 1}, { 1, 0}, };
	EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({ {1,0}, {0,1}, {1,0} }));
	EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({ {1,0}, {0,1}, {1,0} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Gaussian_Synthesis, Back_propagation) {
	synthesis::linear_op<bool> mat{ { 1, 1}, { 0, 1}, };
	EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({ {1,0} }));
	EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({ {1,0} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Gaussian_Synthesis, 3_Qubit) {
	synthesis::linear_op<bool> mat{ { 1, 0, 0}, {1, 1, 0}, {0, 1, 1} };
	EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({ {1,2}, {0,1} }));
	EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({ {1,2}, {0,1} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Gauss, Base) {
	synthesis::linear_op<bool> mat{ {1,0,0,0,0,0,0,0}, 
	                                {1,1,0,0,0,0,0,0}, 
	                                {0,0,1,0,0,0,0,0},
									{0,0,0,1,0,0,0,0},
									{1,0,0,0,1,0,0,0},
									{0,0,0,0,0,1,0,0},
									{0,0,0,0,0,0,1,0},
									{0,0,0,0,0,0,0,1} };

	EXPECT_EQ(synthesis::steiner_gauss(mat, test_device), circuit({ {1,4}, {0,1}, {1,4} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Gauss, Base_inv) {
	synthesis::linear_op<bool> mat{ {1,1,0,0,0,0,0,0}, 
	                                {0,1,0,0,1,0,0,0}, 
	                                {0,0,1,0,0,0,0,0},
									{0,0,0,1,0,0,0,0},
									{0,0,0,0,1,0,0,0},
									{0,0,0,0,0,1,0,0},
									{0,0,0,0,0,0,1,0},
									{0,0,0,0,0,0,0,1} };

	EXPECT_EQ(synthesis::steiner_gauss(mat, test_device), circuit({ {1,0}, {4,1}, {1,0}, {1,0} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Gauss, Fill_Flush) {
	synthesis::linear_op<bool> mat{ {1,0,0,0,0,0,0,0,0},
									{0,1,0,0,0,0,0,0,0},
									{1,0,1,0,0,0,0,0,0},
									{0,0,0,1,0,0,0,0,0},
									{0,0,0,0,1,0,0,0,0},
									{0,0,0,0,0,1,0,0,0},
									{1,0,0,0,0,0,1,0,0},
									{0,0,0,0,0,0,0,1,0},
									{0,0,0,0,0,0,0,0,1} };

	EXPECT_EQ(synthesis::steiner_gauss(mat, test_device), 
		      circuit({ {1,4},{4,7},{7,6},{1,2},{4,7},{1,4},{0,1},{1,4},{4,7},{7,6},{1,2},{4,7},{1,4},{0,1} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Gauss, Swap_Rows) {
	synthesis::linear_op<bool> mat{ {0,1,0,0,0,0,0,0}, 
	                                {1,0,0,0,0,0,0,0}, 
	                                {0,0,1,0,0,0,0,0},
									{0,0,0,1,0,0,0,0},
									{0,0,0,0,1,0,0,0},
									{0,0,0,0,0,1,0,0},
									{0,0,0,0,0,0,1,0},
									{0,0,0,0,0,0,0,1} };

	EXPECT_EQ(synthesis::steiner_gauss(mat, test_device), circuit({ {1,0}, {0,1}, {1,0} }));
}
/******************************************************************************/

/******************************************************************************/
TEST(Steiner_Gauss, Swap_Rows_Nonadjacent) {
	synthesis::linear_op<bool> mat{ {0,0,1,0,0,0,0,0}, 
	                                {0,1,0,0,0,0,0,0}, 
	                                {1,0,0,0,0,0,0,0},
									{0,0,0,1,0,0,0,0},
									{0,0,0,0,1,0,0,0},
									{0,0,0,0,0,1,0,0},
									{0,0,0,0,0,0,1,0},
									{0,0,0,0,0,0,0,1} };

	EXPECT_EQ(synthesis::steiner_gauss(mat, test_device),
              circuit({ {2,1}, {1,0}, {1,2}, {2,1}, {0,1}, {1,2}, {1,0}, {2,1} }));
}
/******************************************************************************/
