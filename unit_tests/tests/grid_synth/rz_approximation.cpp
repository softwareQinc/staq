#include "gtest/gtest.h"

#include "grid_synth/rz_approximation.hpp"

using namespace staq;
using namespace grid_synth;

TEST(RzApproximation, Range) {
    real_t theta_init = PI / 128;
    real_t eps = 1e-3;

    for (int_t i = 0; i < 128; i++) {
        RzApproximation rz_approx = find_rz_approximation(theta_init * i, eps);

        EXPECT_TRUE(rz_approx.solution_found());
        EXPECT_TRUE(rz_approx.error() <= eps);
    }
}
