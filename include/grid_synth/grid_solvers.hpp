#ifndef GRID_SOLVERS_HPP
#define GRID_SOLVERS_HPP

#include <array>
#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>

#include "constants.hpp"
#include "gmp_functions.hpp"
#include "regions.hpp"
#include "rings.hpp"
#include "states.hpp"

namespace staq{
namespace grid_synth {

/**
 * Calculates the lower bound on the integer a for the grid problem solution.
 * The parameter tol sets the tolerance for a to be rounded down instead of up.
 */
template <typename bound_t>
inline int_t lower_bound_a(const real_t& xlo, const int_t& b,
                           const real_t& tol) {
    using namespace std;
    real_t lowera_double = xlo - (b*SQRT2);
    real_t decimal;
    int_t intpart;
    decimal = abs(decimal_part(lowera_double, intpart));
    if ((lowera_double < 0) && ((1 - decimal) < tol))
        return floor(lowera_double);
    if ((lowera_double > 0) && (decimal < tol))
        return floor(lowera_double);

    return ceil(lowera_double);
}

/**
 * Calculates the upper bound on the integer a for the grid problem solution.
 * The parameter tol sets the tolerance for a to be rounded up instead of down.
 */
template <typename bound_t>
inline int_t upper_bound_a(const bound_t& xhi, const int_t& b,
                           const real_t& tol) {
    using namespace std;
    real_t uppera_double = xhi - (b*SQRT2);
    real_t decimal;
    int_t intpart;
    decimal = abs(decimal_part(uppera_double, intpart));
    if ((uppera_double > 0) && ((1 - decimal) < tol))
        return ceil(uppera_double);

    return floor(uppera_double);
}

/**
 * Calculates the lower bound on the parameter b for the grid problem solution,
 * The parameter tol sets the tolerance for b to be rounded down instead of up.
 */
template <typename bound_t>
inline int_t lower_bound_b(const bound_t& xlo, const bound_t& yhi,
                           const bound_t& tol) {
    using namespace std;
    real_t lowerb_double = (xlo - yhi) * HALF_INV_SQRT2;
    real_t decimal;
    int_t intpart;
    decimal = abs(decimal_part(lowerb_double, intpart));
    if ((lowerb_double) < 0 && ((1 - decimal) < tol))
        return floor(lowerb_double);
    if ((lowerb_double) > 0 && (decimal < tol))
        return floor(lowerb_double);

    return ceil(lowerb_double);
}

/**
 * Calculates the upper bound on the parameter b for the grid problem solution.
 * The parameter tol sets the tolerance for b to be rounded up instead of down.
 */
template <typename bound_t>
inline int_t upper_bound_b(const bound_t& xhi, const bound_t& ylo,
                           const real_t& tol) {
    using namespace std;
    real_t upperb_double = (xhi - ylo) * HALF_INV_SQRT2;
    real_t decimal;
    int_t intpart;
    decimal = abs(decimal_part(upperb_double, intpart));
    if ((upperb_double > 0) && (1 - decimal < tol))
        return ceil(upperb_double);

    return floor(upperb_double);
}

/**
 * Finds the smallest exponent k such that the interval width delta = hi-lo
 * obeys,
 *
 *      lambda^(-1) < delta < 1
 *
 * with lambda = (1 + SQRT2)
 */
template <typename bound_t>
inline int_t find_scale_exponent(const Interval<bound_t>& interval) {
    real_t ratio = log(interval.width()) / LOG_LAMBDA;
    return int_t(ratio) + 1;
}

/*
 * Solves the 1D grid problem for the two intervals A and B. The variable tol is
 * used when determining the equality of floats in order to check candidate
 * solutions at the boundaries of the intervals. It's default value is set in
 * constants.hpp to 1e-15, and was set to ensure that certain edge case tests
 * pass.
 */
template <typename bound_t>
inline zsqrt2_vec_t oneD_grid_solver(const Interval<bound_t>& A,
                                     const Interval<bound_t>& B,
                                     const real_t tol = TOL) {

    using namespace std;
    zsqrt2_vec_t solns;
    int_t lowerb = lower_bound_b<bound_t>(A.lo(), B.hi(), tol);
    int_t upperb = upper_bound_b<bound_t>(A.hi(), B.lo(), tol);

    int_t total_a_candidates = 0;
    for (int_t b = lowerb; b <= upperb; b++) {
        int_t lowera = lower_bound_a<bound_t>(A.lo(), b, tol);
        int_t uppera = upper_bound_a<bound_t>(A.hi(), b, tol);
        if(uppera > lowera) {
            total_a_candidates++; 
        }
      
        for (int_t a = lowera; a <= uppera; a++) {
            ZSqrt2 candidate(a, b);
            if (A.contains(candidate.decimal()) and
                B.contains(candidate.decimal_dot())) {
                solns.push_back(candidate);
            }
        }
    }
    return solns;
}

/*
 * Solves the scaled 1D grid problem for the two intervals A and B. The variable
 * tol is used when determining the equality of floats in order to check
 * candidate solutions at the boundaries of the intervals. It's default value is
 * set in constants.hpp to 1e-15, and was set to ensure that certain edge case
 * tests pass.
 *
 * The interval A is scaled to have a width between LAMBDA_INV and 1.
 */
template <typename bound_t>
inline zsqrt2_vec_t oneD_scaled_grid_solver(const Interval<bound_t>& A,
                                            const Interval<bound_t>& B,
                                            const real_t tol = TOL) {
    using namespace std;
   
    zsqrt2_vec_t solns;
    int_t k = find_scale_exponent(A);
    Interval<bound_t> scaled_A = A;
    Interval<bound_t> scaled_B = B;
    if(k>0) {
        scaled_A = A * pow(LAMBDA_INV, k).decimal();
        scaled_B = B * pow(-1 * LAMBDA, k).decimal();
    } else {
        scaled_A = A * pow(LAMBDA, -k).decimal();
        scaled_B = B * pow(-1 * LAMBDA_INV, -k).decimal();
    }
    int_t lowerb = lower_bound_b<bound_t>(scaled_A.lo(), scaled_B.hi(), tol);
    int_t upperb = upper_bound_b<bound_t>(scaled_A.hi(), scaled_B.lo(), tol);
    for (int_t b = lowerb; b <= upperb; b++) {
        int_t lowera = lower_bound_a<bound_t>(scaled_A.lo(), b, tol);
        int_t uppera = upper_bound_a<bound_t>(scaled_A.hi(), b, tol);
        for (int_t a = lowera; a <= uppera; a++) {
            ZSqrt2 candidate(a, b);
            if (scaled_A.contains(candidate.decimal()) and
                scaled_B.contains(candidate.decimal_dot())) {
                if(k>0) {
                    solns.push_back(candidate * pow(LAMBDA, k));
                } else {
                    solns.push_back(candidate * pow(LAMBDA_INV, -k));
                }
            }
        }
    }

    return solns;
}

template <typename bound_t>
inline zsqrt2_vec_t oneD_optimal_grid_solver(const Interval<bound_t>& A,
                                             const Interval<bound_t>& B,
                                             const real_t tol=TOL) {
    return oneD_scaled_grid_solver(A,B,tol);
    //if (A.width() > B.width())
    //    return oneD_scaled_grid_solver(A, B, tol);
    //return oneD_grid_solver(A, B, tol);
}

template <typename bound_t>
inline zomega_vec_t twoD_grid_solver(const UprightRectangle<bound_t> A,
                                     const UprightRectangle<bound_t> B,
                                     const real_t tol = TOL) {
    using namespace std;
  
    zomega_vec_t solns;
    if(A.x_interval().width()*B.x_interval().width() < 1)
      return solns;
    if(B.y_interval().width()*A.y_interval().width()<1)
      return solns;
    
    zsqrt2_vec_t alpha_solns =
        oneD_optimal_grid_solver(A.x_interval(), B.x_interval(), tol);
    zsqrt2_vec_t beta_solns =
        oneD_optimal_grid_solver(A.y_interval(), B.y_interval(), tol);

    zsqrt2_vec_t shifted_alpha_solns = oneD_optimal_grid_solver(
        A.x_interval() - INV_SQRT2, B.x_interval() + INV_SQRT2, tol);
    zsqrt2_vec_t shifted_beta_solns = oneD_optimal_grid_solver(
        A.y_interval() - INV_SQRT2, B.y_interval() + INV_SQRT2, tol);

    for (auto alpha_soln : alpha_solns) {
        for (auto beta_soln : beta_solns) {
            solns.push_back(ZOmega(alpha_soln, beta_soln, 0));
        }
    }
    for (auto alpha_soln : shifted_alpha_solns) {
        for (auto beta_soln : shifted_beta_solns) {
            solns.push_back(ZOmega(alpha_soln, beta_soln, 1));
        }
    }
    return solns;
}

inline zomega_vec_t twoD_grid_solver_ellipse(const Ellipse& A, const Ellipse& B,
                                             const real_t tol = TOL) {
    UprightRectangle<real_t> bboxA = A.bounding_box();
    UprightRectangle<real_t> bboxB = B.bounding_box();

    zomega_vec_t candidates = twoD_grid_solver<real_t>(bboxA, bboxB, tol);
    zomega_vec_t solns;
    for (auto candidate : candidates) {
        if (A.contains(candidate.decimal()) and
            B.contains(candidate.dot().decimal()))
            solns.push_back(candidate);
    }

    return solns;
}

inline zomega_vec_t twoD_grid_solver_ellipse(const state_t& state,
                                             const real_t tol = TOL) {
    using namespace std;
    UprightRectangle<real_t> bboxA = state[0].bounding_box();
    UprightRectangle<real_t> bboxB = state[1].bounding_box();
    cout << fixed << setprecision(100) << bboxA << endl;
    cout << fixed << setprecision(100) << bboxB << endl;
    zomega_vec_t candidates = twoD_grid_solver<real_t>(bboxA, bboxB, tol);
    zomega_vec_t solns;
    for (auto candidate : candidates) {
        if (state[0].contains(candidate.decimal()) and
            state[1].contains(candidate.dot().decimal())) {
            solns.push_back(candidate);
        }
    }

    return solns;
}

/*
 *  Solves the twoD grid problem while fattening the intervals by an amount eps.
 */
inline zomega_vec_t twoD_grid_solver_ellipse_fatten(const state_t& state,
                                                    const real_t& eps,
                                                    const real_t tol = TOL) {
    using namespace std;
    UprightRectangle<real_t> bboxA = state[0].bounding_box().fatten(eps);
    UprightRectangle<real_t> bboxB = state[1].bounding_box().fatten(eps);
    zomega_vec_t candidates = twoD_grid_solver<real_t>(bboxA, bboxB, tol);
    zomega_vec_t solns;
    for (auto candidate : candidates) {
        if (state[0].contains(candidate.decimal()) and
            state[1].contains(candidate.dot().decimal())) {
            solns.push_back(candidate);
        }
    }

    return solns;
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SOLVERS_HPP
