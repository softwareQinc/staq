
#include "gtest/gtest.h"

#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/types.hpp"

using namespace staq;
using namespace grid_synth;

TEST(ExactSynthesis, RangeOfAngle) {
    domega_matrix_table_t s3_table = generate_s3_table();

    real_t eps = 1e-3;
    real_t phi = PI / 128;

    for (int i = 0; i < 128; i++) {
        RzApproximation rz_approx = find_rz_approximation(phi, eps);
        str_t op_str = synthesize(rz_approx.matrix(), s3_table);

        EXPECT_TRUE(domega_matrix_from_str(op_str) == rz_approx.matrix());
    }
}
