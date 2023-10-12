#include "gtest/gtest.h"

#include "grid_synth/gmp_functions.hpp"

using namespace staq;
using namespace gmpf;

TEST(GmpFunctions, min) {
    mpz_class x("10000012031201301030102301301023012301030103010301301023010101"
                "03010310131");
    mpz_class y("12398198091845901809458349085918340581034850193845019839405810"
                "39485091834");

    EXPECT_TRUE(gmp_min(x, y) == x);
    EXPECT_TRUE(gmp_min(y, x) == x);

    x = mpz_class("-13490123481023498091285094850984350980594810938450938409581"
                  "8934514908249038290384");
    y = mpz_class("-12910905890348590384509183045983019458091384509384509384509"
                  "8130458103485013845094");

    EXPECT_TRUE(gmp_min(x, y) == x);
    EXPECT_TRUE(gmp_min(y, x) == x);

    x = mpz_class("-51509183490598134905810934580913458091385039845093485093845"
                  "0938450039485093850914");
    y = mpz_class("394158094850934850913485903485019438590318501384509384509384"
                  "5093840598103985091350");

    EXPECT_TRUE(gmp_min(x, y) == x);
    EXPECT_TRUE(gmp_min(y, x) == x);
}

TEST(GmpFunctions, max) {
    mpz_class x = mpz_class("10000012031201301030102301301023012301030103010301"
                            "30102301010103010310131");
    mpz_class y = mpz_class("12398198091845901809458349085918340581034850193845"
                            "01983940581039485091834");

    EXPECT_TRUE(gmp_max(x, y) == y);
    EXPECT_TRUE(gmp_max(y, x) == y);

    x = mpz_class("-13490123481023498091285094850984350980594810938450938409581"
                  "8934514908249038290384");
    y = mpz_class("-12910905890348590384509183045983019458091384509384509384509"
                  "8130458103485013845094");

    EXPECT_TRUE(gmp_max(x, y) == y);
    EXPECT_TRUE(gmp_max(y, x) == y);

    x = mpz_class("-51509183490598134905810934580913458091385039845093485093845"
                  "0938450039485093850914");
    y = mpz_class("394158094850934850913485903485019438590318501384509384509384"
                  "5093840598103985091350");

    EXPECT_TRUE(gmp_max(x, y) == y);
    EXPECT_TRUE(gmp_max(y, x) == y);
}

TEST(GmpFunctions, floor) {
    mpf_class x(
        "-1.123153451345634647367356735673567357635673567356735735673573573");

    EXPECT_TRUE(gmp_floor(x) == -2);

    x = mpf_class("1."
                  "213341545346456345647667356736573657356736573573567356735673"
                  "5735735735");

    EXPECT_TRUE(gmp_floor(x) == 1);
}

TEST(GmpFunctions, ceil) {
    mpf_class x(
        "-1.123153451345634647367356735673567357635673567356735735673573573");

    EXPECT_TRUE(gmp_ceil(x) == -1);

    x = mpf_class("1."
                  "213341545346456345647667356736573657356736573573567356735673"
                  "5735735735");

    EXPECT_TRUE(gmp_ceil(x) == 2);
}

TEST(GmpFunctions, round) {
    mpf_set_default_prec(4096);
    mpf_class x(
        "-1.5452624566272736757567577777777777777777666666666666666666666");

    EXPECT_TRUE(gmp_round(x) == -2);

    x = mpf_class("-12001894518450983940850238599034852093502983049520923580385"
                  "20385.04345234523523452345345");

    EXPECT_TRUE(gmp_round(x) ==
                mpz_class("-120018945184509839408502385990348520935"
                          "0298304952092358038520385"));

    x = mpf_class("1349218409."
                  "231940295801948509438509834095810394859013845091384095810934"
                  "580193845091385");

    EXPECT_TRUE(gmp_round(x) == mpz_class("1349218409"));

    x = mpf_class(
        "4935810934580938."
        "66980981094819028419028409184309138240928409890902941390490284");

    EXPECT_TRUE(gmp_round(x) == mpz_class("4935810934580939"));
}

TEST(GmpFunctions, exp) {
    long int prec = 256;
    mpf_set_default_prec(prec);
    // The -2 is a little hacky, but it is necessary to have the tests pass
    //  (we are testing absolute error instead of relative error, and the
    //   largest values are roughly in the range 10^2).
    long int tol_exp = std::log10(2) * prec - 2;
    mpf_class eps(("1e-" + std::to_string(tol_exp)));
    // Expect values are calculated using Wolfram Alpha to 100 digits.
    // log(2) * 256 is approx 77, so 100 digits is accurate enough.

    std::vector<std::pair<std::string, std::string>> cases{
        {"0", "1"},
        {"1", "2."
              "7182818284590452353602874713526624977572470936999595749"
              "66967627724076630353547594571382178525166427"},
        {"-1", "0."
               "367879441171442321595523770161460867445811131031767834507836801"
               "6974614957448998033571472743459196437"},
        {"-0.1234567",
         "0."
         "883859911549690424603734186208757339780798792486720427068041849393"
         "9612541057720515407769091940206197"},
        {"5.623478", "276."
                     "8505970916278258711936698732987836757702032228446903804"
                     "870918696416770256055219817409072316698596"},
        /* This case will fail because the absolute error is too large
         * (although, the relative error is still bounded by epsilon).
         *      {"100", "26881171418161354484126255515800135873611118."
         *              "77374192241519160861528028703490956491415887109721984571"},
         */
        {"-100", "0."
                 "0000000000000000000000000000000000000000000372007597602083596"
                 "2959695803863118337358892292376781967120613876663290475895815"
                 "718157118778642281497"}};

    for (auto& [x, expect] : cases) {
        EXPECT_TRUE(gmp_abs(gmpf::exp(mpf_class(x)) - mpf_class(expect)) < eps);
    }
}
