#include "grid_synth/grid_synth.hpp"
#include "gtest/gtest.h"

using namespace staq;
using namespace grid_synth;

// Synthesize multiples of pi/4
TEST(GridSynth, ExactSynthesis) {
    GridSynthOptions opt{100, 200, false, false, false, false};
    GridSynthesizer synthesizer = make_synthesizer(opt);

    for (int i = -20; i <= 20; ++i) {
        real_t angle = real_t(i) / real_t(4);
        str_t op_str = synthesizer.get_op_str(angle * gmpf::gmp_pi());
        real_t eps = gmpf::pow(real_t(10), -100);
        str_t common_case = check_common_cases(angle, eps);
        EXPECT_TRUE(op_str == common_case);
    }
}

// Synthesize other angles
TEST(GridSynth, InexactSynthesis) {
    GridSynthOptions opt{100, 200, false, false, false, false};
    GridSynthesizer synthesizer = make_synthesizer(opt);
    EXPECT_TRUE(synthesizer.is_valid());

    synthesizer.get_op_str(real_t("0.3"));
    EXPECT_TRUE(synthesizer.is_valid());

    synthesizer.get_op_str(real_t("0.3"));
    EXPECT_TRUE(synthesizer.is_valid());

    synthesizer.get_op_str(real_t("5.3423"));
    EXPECT_TRUE(synthesizer.is_valid());

    synthesizer.get_op_str(real_t("-5.3123"));
    EXPECT_TRUE(synthesizer.is_valid());
}
