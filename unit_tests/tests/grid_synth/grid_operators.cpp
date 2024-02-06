#include "gtest/gtest.h"

#include "staq/grid_synth/grid_operators.hpp"
#include "staq/grid_synth/rings.hpp"

using namespace staq;
using namespace grid_synth;

TEST(GridOperator, MultiplicationWithZOmega) {
    SpecialGridOperator G(1, 0, 0, 2, 0, 0, 1, 0);
    ZOmega Unit(0, 0, 0, 1);
    ZOmega A(0, 1, 0, 1);

    EXPECT_TRUE(G * Unit == Unit);
    EXPECT_TRUE(G * A == ZOmega(-1, 1, 1, 1));
}

TEST(SpecialGridOperator, Inverse) {
    SpecialGridOperator G(1, 0, 0, 2, 0, 0, 1, 0);

    EXPECT_TRUE(G.inverse() * G == ID);
    EXPECT_TRUE(Z.inverse() * X.inverse() * K.inverse() * B(1).inverse() *
                    A(1).inverse() * R.inverse() * R * A(1) * B(1) * K * X *
                    Z ==
                ID);
}

TEST(SpecialGridOperator, InverseIsIdentity) {
    SpecialGridOperator G(1, 0, 0, 2, 0, 0, 1, 0);

    ZOmega X(4, 3, 2, 10);
    EXPECT_TRUE(G.inverse() * (G * X) == X);

    X = ZOmega(-5, -10, 1, 2);
    EXPECT_TRUE(G.inverse() * (G * X) == X);
}

TEST(SpecialGridOperator, ConjugateEqualsShift) {
    EXPECT_TRUE(R.conjugate() == shift(R, 1));
    EXPECT_TRUE(R.conjugate().conjugate() == shift(R, 2));

    EXPECT_TRUE(R.inv_conjugate() == shift(R, -1));
    EXPECT_TRUE(R.inv_conjugate().inv_conjugate() == shift(R, -2));
}
