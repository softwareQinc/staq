#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cmath>
#include <iostream>

#include "types.hpp"
#include "gmp_functions.hpp"
#include "complex.hpp"

namespace staq {
namespace grid_synth {


struct MultiPrecisionConstants {
    real_t tol;
    real_t pi;
    long int default_gmp_prec;
    real_t sqrt2;
    real_t inv_sqrt2;
    real_t half_inv_sqrt2;
    cplx_t omega;
    cplx_t omega_conj;
    cplx_t im;
    real_t log_lambda;
    real_t sqrt_lambda;
    real_t sqrt_lambda_inv;
};

inline MultiPrecisionConstants initialize_constants(long int prec) {
    long int default_gmp_prec = 4 * prec + 19;
    mpf_set_default_prec(log2(10) * default_gmp_prec);
    real_t tol = gmpf::pow(real_t(10), -default_gmp_prec + 2);
    real_t pi = gmpf::gmp_pi();
    real_t sqrt2 = gmpf::sqrt(real_t(2));
    real_t inv_sqrt2 = real_t(real_t(1) / sqrt2);
    real_t half_inv_sqrt2 = real_t(real_t(1) / (real_t(2) * sqrt2));
    cplx_t omega = cplx_t(inv_sqrt2, inv_sqrt2);
    cplx_t omega_conj = cplx_t(inv_sqrt2, -inv_sqrt2);
    real_t log_lambda = gmpf::log10(real_t(1)+sqrt2);
    real_t sqrt_lambda = gmpf::sqrt(real_t(1)+sqrt2);
    real_t sqrt_lambda_inv = sqrt(-real_t(1) + sqrt2);
    cplx_t im = cplx_t(real_t(0), real_t(1));

    return MultiPrecisionConstants{
              tol,pi,default_gmp_prec,sqrt2,inv_sqrt2,half_inv_sqrt2,omega,omega_conj,im,
              log_lambda, sqrt_lambda, sqrt_lambda_inv};
}

inline MultiPrecisionConstants MP_CONSTS = initialize_constants(10);

#define TOL MP_CONSTS.tol
#define PI MP_CONSTS.pi
#define DEFAULT_GMP_PREC MP_CONSTS.default_gmp_prec
#define SQRT2 MP_CONSTS.sqrt2
#define INV_SQRT2 MP_CONSTS.inv_sqrt2
#define HALF_INV_SQRT2 MP_CONSTS.half_inv_sqrt2
#define OMEGA MP_CONSTS.omega
#define OMEGA_CONJ MP_CONSTS.omega_conj
#define Im MP_CONSTS.im
#define LOG_LAMBDA MP_CONSTS.log_lambda
#define SQRT_LAMBDA MP_CONSTS.sqrt_lambda
#define SQRT_LAMBDA_INV MP_CONSTS.sqrt_lambda_inv

inline int MAX_ATTEMPTS_POLLARD_RHO = 200;

const int KMIN = 0;
const int KMAX = 10000000;
const int COLW = 10;
const int PREC = 5;
const int POLLARD_RHO_INITIAL_ADDEND = 1;
const int POLLARD_RHO_START = 2;
const int MOD_SQRT_MAX_DEPTH = 20;

const int MAX_ITERATIONS_FERMAT_TEST = 5;
const str_t DEFAULT_TABLE_FILE = "./.s3_table_file.csv";

// on average we only need 2 attempts so 5 is playing it safe
const int MAX_ATTEMPTS_SQRT_NEG_ONE = 100;

} // namespace grid_synth
} // namespace staq

#endif // CONSTANTS_HPP
