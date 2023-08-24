#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cmath>
#include <complex>
#include <iostream>

#include "types.hpp"

namespace staq {
namespace grid_synth {

// Tolerance for equality when comparing floats. Default is set to guarentee
// known edge cases.
real_t TOL = 1e-17;
real_t PI = M_PI;
long int DEFAULT_GMP_PREC = 500;
real_t SQRT2 = sqrt(real_t(2));
real_t INV_SQRT2 = real_t(1) / SQRT2;
real_t HALF_INV_SQRT2 = real_t(1) / (real_t(2) * SQRT2);
cplx_t OMEGA(INV_SQRT2, INV_SQRT2);
cplx_t OMEGA_CONJ(INV_SQRT2, -INV_SQRT2);
cplx_t Im(real_t(0), real_t(1));

const double LOW_PREC_TOL = 1e-17; // tolerance for float equality as low
                                   // precisions

const int KMIN = 0; // Default minimum k value
const int KMAX = 1000; // Default maximum k value
const int COLW = 10; // Default width of columns for data output
const int PREC = 5;  // Default precision for output
const int MAX_ATTEMPTS_POLLARD_RHO = 10000000;
const int POLLARD_RHO_INITIAL_ADDEND = 1;
const int POLLARD_RHO_START = 2;
const int MOD_SQRT_MAX_DEPTH = 20;

const int MAX_ITERATIONS_FERMAT_TEST = 5;
const str_t DEFAULT_TABLE_FILE="./s3_table_file.csv";


// on average we only need 2 attempts so 5 is playing it safe
const int MAX_ATTEMPTS_SQRT_NEG_ONE = 10;
        

/*
 *  These need to be initialized once the required precision is known. 
 */
//class MultiPrecisionConstants {
//    private:
//        inline static long int gmp_prec_; // precision of gmp ints
//        inline static real_t tol_; // tolerance for float equaliy and zero checking
//        inline static real_t pi_;
//        inline static real_t sqrt2_;
//        inline static real_t inv_sqrt2_;
//        inline static real_t half_inv_sqrt2_;
//        inline static cplx_t omega_;
//        inline static cplx_t omega_conj_;
//        inline static cplx_t im_;
//        
//        inline static mpf_class gmp_pi_(const mpf_class& tol) {
//            using namespace std;
//            real_t three("3");
//            real_t lasts("0");
//            real_t t = three;
//            real_t s("3");
//            real_t n("1");
//            real_t na("0");
//            real_t d("0"); 
//            real_t da("24");
//            while(abs(s-lasts)>tol) {
//                lasts = s;
//                n = n+na;
//                na = na + mpf_class("8");
//                d = d+da;
//                da = da+mpf_class("32");
//                t = (t*n) / d;
//                s += t;
//            }
//            return s;
//        }
//
//    public:
//        /*
//         *  Accepts the precision required for the solution of the approximate
//         *  synthesis problem
//         */
//        static void initialize(long int prec) {
//            gmp_prec_ = -4*prec-19;
//            mpf_set_default_prec(gmp_prec_);
//            tol_ = real_t(str_t(-4*prec-16,'0')+"1");
//            pi_ = gmp_pi_(tol_);
//            sqrt2_ = sqrt(real_t(2));
//            inv_sqrt2_ = real_t(1) / sqrt2_;
//            half_inv_sqrt2_ = real_t(1) / (real_t(2)*sqrt2_);
//            omega_ = cplx_t(inv_sqrt2_,inv_sqrt2_);
//            omega_conj_ = cplx_t(inv_sqrt2_, -inv_sqrt2_); 
//            im_ = cplx_t(real_t(0),real_t(1));
//        }
//        
//        static long int gmp_prec() noexcept { return gmp_prec_; }
//        static real_t tol() noexcept { return tol_; }
//        static real_t pi() noexcept { return pi_; }
//        static real_t sqrt2() noexcept { return sqrt2_; }
//        static real_t inv_sqrt2() noexcept { return inv_sqrt2_; }
//        static real_t half_inv_sqrt2() noexcept { return half_inv_sqrt2_; }
//        static cplx_t omega() noexcept { return omega_; }
//        static cplx_t omega_conj() noexcept { return omega_conj_; }
//        static cplx_t im() noexcept { return im_; }
//};


//#define PI MultiPrecisionConstants::pi()
//#define TOL MultiPrecisionConstants::tol()
//#define DEFAULT_GMP_PRECISION MultiPrecisionConstants::gmp_prec()
//#define SQRT2 MultiPrecisionConstants::sqrt2()
//#define INV_SQRT2 MultiPrecisionConstants::inv_sqrt2()
//#define HALF_INV_SQRT2 MultiPrecisionConstants::half_inv_sqrt2()
//#define OMEGA MultiPrecisionConstants::omega()
//#define OMEGA_CONJ MultiPrecisionConstants::omega_conj()
//#define Im MultiPrecisionConstants::im()


} // namespace grid_synth
} // namespace staq
  
#endif // CONSTANTS_HPP
