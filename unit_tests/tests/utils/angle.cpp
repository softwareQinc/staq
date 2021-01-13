#include "gtest/gtest.h"
#include "utils/angle.hpp"

using namespace staq::utils;

// Tiny unit tests for angles

/******************************************************************************/
TEST(Angles, Pi) { EXPECT_EQ(angles::pi.numeric_value(), pi); }
/******************************************************************************/

/******************************************************************************/
TEST(Angles, Symbolic_Symbolic) {
    EXPECT_TRUE((angles::pi_quarter + angles::pi_half).is_symbolic());
    EXPECT_TRUE((angles::pi_quarter - angles::pi_half).is_symbolic());
    EXPECT_TRUE((angles::pi_quarter * 2).is_symbolic());
    EXPECT_TRUE((angles::pi_quarter / 2).is_symbolic());
}
/******************************************************************************/

/******************************************************************************/
TEST(Angles, Symbolic_Numeric) {
    EXPECT_TRUE((angles::pi_quarter + Angle(1.1)).is_numeric());
    EXPECT_TRUE((Angle(1.1) - angles::pi_quarter).is_numeric());
}
/******************************************************************************/

/******************************************************************************/
TEST(Angles, Arithmetic) {
    EXPECT_EQ(-angles::pi_quarter, Angle(7, 4));
    EXPECT_EQ(-angles::pi_quarter,
              angles::pi_quarter + angles::pi_half + angles::pi);
    EXPECT_EQ(angles::pi_quarter / 2, Angle(1, 8));
    EXPECT_EQ(angles::pi_quarter * 2, angles::pi_half);
}
/******************************************************************************/
