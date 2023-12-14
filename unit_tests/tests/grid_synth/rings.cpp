#include "gtest/gtest.h"

#include "staq/grid_synth/rings.hpp"

using namespace staq;
using namespace grid_synth;

TEST(ZSqrt2Comparison, EqualsFalse) {
    EXPECT_FALSE(ZSqrt2(-9, 11) == ZSqrt2(11, 1));
}

TEST(ZSqrt2Comparison, EqualsTrue) {
    EXPECT_TRUE(ZSqrt2(-1, 12) == ZSqrt2(-1, 12));
}

TEST(ZSqrt2Comparison, NotEqualsTrue) {
    EXPECT_TRUE(ZSqrt2(8, -123) != ZSqrt2(-34, -12));
}

TEST(ZSqrt2Comparison, NotEqualsFalse) {
    EXPECT_FALSE(ZSqrt2(9, 9) != ZSqrt2(9, 9));
}

TEST(ZSqrt2Comparison, GreaterThanZSqrt2) {
    EXPECT_TRUE(ZSqrt2(8, 8) > ZSqrt2(-12, 2));
    EXPECT_FALSE(ZSqrt2(8, 4) > ZSqrt2(10, 12));
}

TEST(ZSqrt2Comparison, LessThanZSqrt2) {
    EXPECT_TRUE(ZSqrt2(12, 3) < ZSqrt2(100, 10));
    EXPECT_FALSE(ZSqrt2(100, 1200) < ZSqrt2(-120, 2));
}

TEST(ZSqrt2Comparison, GreaterThanDecimal) {
    EXPECT_TRUE(120.34 > ZSqrt2(1, 4));
    EXPECT_FALSE(10.0 > ZSqrt2(50, 100));
}

TEST(ZSqrt2Arithmetic, Addition) {
    EXPECT_TRUE(ZSqrt2(1, 1) + ZSqrt2(1, 1) == ZSqrt2(2, 2));
}

TEST(ZSqrt2Arithmetic, Subtraction) {
    EXPECT_TRUE(ZSqrt2(1, 1) - ZSqrt2(1, 1) == ZSqrt2(0, 0));
    EXPECT_TRUE(ZSqrt2(-10, 100) - ZSqrt2(-10, 100) == ZSqrt2(0, 0));
}

TEST(ZSqrt2Arithmetic, Multiplication) {
    ZSqrt2 X(1, 3);
    ZSqrt2 Y(2, -4);
    ZSqrt2 Z = X * Y;

    X *= Y;

    EXPECT_TRUE(Z == X);
    EXPECT_TRUE(ZSqrt2(1, 1) * ZSqrt2(-7, 8) == ZSqrt2(9, 1));
}

TEST(ZSqrt2Arithmetic, Exponentiation) {
    EXPECT_TRUE(pow(LAMBDA, 0) == ZSqrt2(1, 0));
    EXPECT_TRUE(pow(LAMBDA, 1) == LAMBDA);
    EXPECT_TRUE(pow(LAMBDA, 2) == LAMBDA * LAMBDA);
}

TEST(ZSqrt2Arithmetic, EuclideanDivision) {
    ZSqrt2 A(10, -5);
    ZSqrt2 B(-3, 1);

    EXPECT_TRUE(A == (A / B) * B + A % B);
}

TEST(ZSqrt2Inverse, LambdaInverse) {
    ZSqrt2 unit = (LAMBDA * LAMBDA_INV);

    EXPECT_TRUE(ZSqrt2(1, 0) == unit);
}

TEST(ZOmegaConstructor, ConstructorEquality) {
    MP_CONSTS.tol = 1e-16;

    ZOmega ztest(5, 6, -2, 1);

    EXPECT_TRUE(ztest.alpha() == ZSqrt2(1, -4));
    EXPECT_TRUE(ztest.beta() == ZSqrt2(6, 1));
    EXPECT_TRUE(ztest.w());

    cplx_t zsqrt2dec(ztest.alpha().decimal(), ztest.beta().decimal());

    zsqrt2dec += cplx_t(ztest.w(), 0) * OMEGA;

    EXPECT_TRUE(abs(ztest.decimal() - zsqrt2dec) < TOL);
}

TEST(ZOmegaArithmetic, Comparison) {
    ZOmega Y(5, 6, -2, 1);
    ZOmega Z(ZSqrt2(1, -4), ZSqrt2(6, 1), 1);

    EXPECT_TRUE(Y == Z);

    Y = ZOmega(-10, 3, 2, 40);

    EXPECT_FALSE(Y == Z);

    Y = Z;

    EXPECT_TRUE(Y == Z);
}
