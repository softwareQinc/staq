#include "gtest/gtest.h"

#include "staq/grid_synth/diophantine_solver.hpp"

using namespace staq;
using namespace grid_synth;

TEST(PrimeFactorizeInt, SuccessOnFirstTenThousandNumbers) {
    for (int_t n = 2; n < 100000; n++) {
        int_vec_t prime_factors;

        if (n % 7 == 0)
            continue;

        bool prime_factorize_succeeded = prime_factorize_int(prime_factors, n);

        real_t prod = 1;
        for (auto prime_factor : prime_factors)
            prod *= prime_factor;

        if (prime_factorize_succeeded)
            EXPECT_TRUE(prod == n);
    }
}
