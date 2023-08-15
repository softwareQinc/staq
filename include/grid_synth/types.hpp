#ifndef TYPES_HPP
#define TYPES_HPP

#include <Eigen/Dense>
#include <complex>
#include <gmpxx.h>
#include <queue>
#include <string>

#include "complex.hpp"

namespace staq {
namespace grid_synth {
using int_t = mpz_class;
using real_t = mpf_class;
using cplx_t = complex<real_t>;
using str_t = std::string;

using vec_t = Eigen::Matrix<real_t, 2, 1>;
using mat_t = Eigen::Matrix<real_t, 2, 2>;

using int_vec_t = std::vector<int_t>;
using int_queue_t = std::queue<int_t>;
} // namespace grid_synth
} // namespace staq

#endif // TYPES_HPP
