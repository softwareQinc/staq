#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cmath>
#include <complex>

#include "types.hpp"

namespace staq {
namespace grid_synth {

const long int DEFAULT_GMP_PREC = 300;

const real_t SQRT2 = sqrt(real_t(2));
const real_t INV_SQRT2 = 1.0 / SQRT2;
const real_t HALF_INV_SQRT2 = 1.0 / (2 * SQRT2);
const real_t PI = M_PI;

const cplx_t OMEGA(INV_SQRT2, INV_SQRT2);
const cplx_t OMEGA_CONJ(INV_SQRT2, -INV_SQRT2);

const cplx_t Im(0, 1);

const int KMIN = 0; // Default minimum k value
const int KMAX = 100; // Default maximum k value
const int COLW = 10; // default width of columns for data output
const int PREC = 5;  // default precision for output
const int_t MAX_ATTEMPTS_POLLARD_RHO = 10000000;
const int POLLARD_RHO_INITIAL_ADDEND = 1;
const int POLLARD_RHO_START = 2;
const int MOD_SQRT_MAX_DEPTH = 20;

const int MAX_ITERATIONS_FERMAT_TEST = 5;
const str_t DEFAULT_TABLE_FILE="./s3_table_file.csv";

// Tolerance for equality when comparing floats. Default is set to guarentee
// known edge cases.
const real_t TOL = 1e-15;

// on average we only need 2 attempts so 5 is playing it safe
const int MAX_ATTEMPTS_SQRT_NEG_ONE = 10;

} // namespace grid_synth
} // namespace staq
  
#endif // CONSTANTS_HPP
