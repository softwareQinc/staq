#ifndef RANDOM_NUMBERS_HPP
#define RANDOM_NUMBERS_HPP

#include <chrono>
#include <gmpxx.h>

namespace staq {
namespace grid_synth {

inline gmp_randclass random_numbers(gmp_randinit_mt);

} // namespace grid_synth
} // namespace staq

#endif // RANDOM_NUMBERS_HPP
