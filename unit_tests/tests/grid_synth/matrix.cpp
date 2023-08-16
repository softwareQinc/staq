#include "gtest/gtest.h"

#include "grid_synth/matrix.hpp"
#include "grid_synth/rz_approximation.hpp"

using namespace staq;
using namespace grid_synth;

TEST(Matrix, Equality) {
    EXPECT_TRUE(I == I);
    EXPECT_TRUE(H == H);
    EXPECT_TRUE(T == T);
    EXPECT_TRUE(S == S);
}

TEST(Matrix, Multiplication) { EXPECT_TRUE(T * T == S); }

TEST(Matrix, Dagger) {
    real_t eps = 1e-3;
    real_t theta = PI / 7;

    for (int_t i = 0; i < 7; i++) {
        RzApproximation rz_approx = find_rz_approximation(theta * i, eps);
        DOmegaMatrix matrix = rz_approx.matrix();
        EXPECT_TRUE(matrix * matrix.dagger() == I);
    }

    EXPECT_TRUE(H == H.dagger());
    EXPECT_TRUE(H * H.dagger() == I);
    EXPECT_TRUE(T * T.dagger() == I);
    EXPECT_TRUE(S * S.dagger() == I);
    EXPECT_TRUE(I == I.dagger());
}

TEST(Matrix, S3Table) {
    domega_matrix_table_t s3_table = generate_s3_table();
    EXPECT_TRUE(str_t("S") == s3_table[T * T]);
    EXPECT_TRUE(str_t("H") == s3_table[H]);
    EXPECT_TRUE(str_t("SHST") == s3_table[S * H * T * T * T]);
}
