#include "gtest/gtest.h"

#include "grid_synth/states.hpp"

using namespace staq;
using namespace grid_synth;

TEST(ShiftState, ShiftToUnity) {
    Ellipse A(0, 0, 10, 5, 0.45 * PI);
    Ellipse B(0, 0, pow(LAMBDA_INV.decimal(), 6), pow(LAMBDA_INV.decimal(), -6),
              0);

    real_t scaleA = A.normalize();
    real_t scaleB = B.normalize();

    int_t k = determine_shift(state_t{A, B});

    state_t shifted_state = shift(state_t{A, B}, k);

    EXPECT_FALSE(bias(shifted_state) > 1);

    A.rescale(1 / scaleA);
    B.rescale(1 / scaleB);
}
