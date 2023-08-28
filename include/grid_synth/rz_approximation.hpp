#ifndef RZ_APPROXIMATION_HPP
#define RZ_APPROXIMATION_HPP

#include <cmath>
#include <gmpxx.h>
#include <iomanip>
#include <iostream>

#include "diophantine_solver.hpp"
#include "gmp_functions.hpp"
#include "grid_operators.hpp"
#include "grid_solvers.hpp"
#include "matrix.hpp"
#include "regions.hpp"
#include "rings.hpp"

namespace staq {
namespace grid_synth {

class RzApproximation {
  private:
    DOmegaMatrix matrix_;

    real_t eps_;
    bool solution_found_;

    cplx_t u_val_;
    cplx_t t_val_;
    cplx_t z_;

  public:
    explicit RzApproximation()
        : matrix_(DOmegaMatrix(ZOmega(0), ZOmega(0), 0, 0)), eps_(0),
          solution_found_(false), z_(cplx_t(0, 0)) {}

    RzApproximation(const ZOmega& u, const ZOmega& t,
                    const int_t scale_exponent, const real_t theta,
                    const real_t eps)
        : matrix_(u, t, scale_exponent, 0), eps_(eps), solution_found_(true) {
        u_val_ = cplx_t(u.decimal().real() / pow(SQRT2, scale_exponent),
                        u.decimal().imag() / pow(SQRT2, scale_exponent));
        t_val_ = cplx_t(t.decimal().real() / pow(SQRT2, scale_exponent),
                        t.decimal().imag() / pow(SQRT2, scale_exponent));
        z_ = cplx_t(cos(theta), sin(theta));
    }

    DOmegaMatrix matrix() const { return matrix_; }

    ZOmega u() const { return matrix_.u(); }
    ZOmega t() const { return matrix_.t(); }

    cplx_t u_val() const { return u_val_; }
    cplx_t t_val() const { return t_val_; }

    int_t scale_exponent() const { return matrix_.k(); }
    real_t eps() const { return eps_; }

    bool solution_found() { return solution_found_; }

    real_t error() {
        return sqrt(((u_val_ - z_).conj() * (u_val_ - z_)).real() +
                    (t_val_.conj() * t_val_).real());
    }
};

inline RzApproximation find_rz_approximation(const real_t& theta,
                                             const real_t& eps,
                                             const real_t tol = TOL) {
    using namespace std;
    if (abs(theta) < TOL) {
        return RzApproximation(ZOmega(1), ZOmega(0), 0, theta, eps);
    }
    // int_t k = 3 * int_t(log(real_t(1) / eps) / log(2)) / 2;
    int_t k = 0;
    int_t max_k = 1000;
    bool solution_found = false;
    zomega_vec_t scaled_candidates;
    vec_t z{std::cos(theta.get_d()), std::sin(theta.get_d())};
    Ellipse eps_region(theta, eps);
    Ellipse disk(0, 0, 1, 1, 0);
    state_t state{eps_region, disk};

    SpecialGridOperator G = optimize_skew(state);

    real_t scaleA;
    real_t scaleB;
    while ((not solution_found) && k < max_k) {
        if (k % 2 == 0) {
            scaleA = pow(2, k / 2);
            scaleB = pow(2, k / 2);
        } else {
            scaleA = pow(2, (k - 1) / 2) * SQRT2;
            scaleB = -pow(2, (k - 1) / 2) * SQRT2;
        }
        state[0].rescale(scaleA);
        state[1].rescale(scaleB);
        scaled_candidates = twoD_grid_solver_ellipse_fatten(state, eps, tol);
        int_t points_in_region = 0;
        for (auto scaled_candidate : scaled_candidates) {
            auto candidate = G * scaled_candidate;
            if (((candidate.real() / scaleA) * z[0] +
                 (candidate.imag() / scaleA) * z[1]) > 1 - (eps * eps / 2)) {
                int_t temp_k = k;
                points_in_region++;
                while (candidate.is_reducible()) {
                    temp_k -= 1;
                    candidate = candidate.reduce();
                }

                ZSqrt2 xi = ZSqrt2(int_t(pow(2, temp_k)), 0) -
                            (candidate.conj() * candidate).to_zsqrt2();
                ZOmega answer(0);
                solution_found = diophantine_solver(answer, xi);
                if (solution_found) {
                    return RzApproximation(candidate, answer, temp_k, theta,
                                           eps);
                }
            }
        }
        k++;
        state[0].rescale(1 / scaleA);
        state[1].rescale(1 / scaleB);
    }
    return RzApproximation();
}

inline RzApproximation find_fast_rz_approximation(const real_t& theta,
                                                  const real_t& eps,
                                                  const int_t& kmin = KMIN,
                                                  const int_t& kmax = KMAX,
                                                  const real_t tol = TOL) {
    using namespace std;
    // int_t k = 3 * int_t(log(real_t(1) / eps) / log(2)) / 2;
    int_t k = kmin;
    int_t max_k = kmax;
    bool solution_found = false;
    zomega_vec_t scaled_candidates;
    vec_t z{cos(theta), sin(theta)};
    Ellipse eps_region(theta, eps);
    Ellipse disk(real_t("0"), real_t("0"), real_t("1"), real_t("1"),
                 real_t("0"));
    state_t state{eps_region, disk};
    SpecialGridOperator G = optimize_skew(state);
    real_t scaleA, scaleB;

    UprightRectangle<real_t> bboxA = state[0].bounding_box();
    UprightRectangle<real_t> bboxB = state[1].bounding_box();

    while ((not solution_found) && (k < max_k)) {
        if (k % 2 == 0) {
            scaleA = pow(2, k / 2);
            scaleB = pow(2, k / 2);
        } else {
            scaleA = pow(2, (k - 1) / 2) * SQRT2;
            scaleB = -pow(2, (k - 1) / 2) * SQRT2;
        }

        bboxA.rescale(scaleA, scaleA);
        bboxB.rescale(scaleB, scaleB);

        Interval<real_t> A_x = bboxA.x_interval().fatten(eps);
        Interval<real_t> B_x = bboxB.x_interval().fatten(eps);

        Interval<real_t> A_y = bboxA.y_interval().fatten(eps);
        Interval<real_t> B_y = bboxB.y_interval().fatten(eps);

        real_t min_width = (1 + SQRT2) * (1 + SQRT2);

        // Skip any iteration that isn't guarenteed to have at least one
        // solution
        // if((A_x.width()*B_x.width()<min_width) or
        // (A_y.width()*B_y.width()<min_width)) {
        //    k++;
        //    bboxA.rescale(1/scaleA,1/scaleA);
        //    bboxB.rescale(1/scaleB,1/scaleB);
        //    continue;
        //}

        zsqrt2_vec_t alpha_solns = oneD_optimal_grid_solver(A_x, B_x, tol);
        zsqrt2_vec_t beta_solns = oneD_optimal_grid_solver(A_y, B_y, tol);

        zsqrt2_vec_t shifted_alpha_solns =
            oneD_optimal_grid_solver(A_x - INV_SQRT2, B_x + INV_SQRT2, tol);
        zsqrt2_vec_t shifted_beta_solns =
            oneD_optimal_grid_solver(A_y - INV_SQRT2, B_y + INV_SQRT2, tol);

        for (auto alpha_soln : alpha_solns)
            for (auto beta_soln : beta_solns) {
                ZOmega candidate = G * ZOmega(alpha_soln, beta_soln, 0);
                if (((candidate.real() / scaleA) * z[0] +
                     (candidate.imag() / scaleA) * z[1]) >
                    real_t("1") - (eps * eps / real_t("2"))) {
                    int_t temp_k = k;
                    while (candidate.is_reducible()) {
                        temp_k -= 1;
                        candidate = candidate.reduce();
                    }

                    ZSqrt2 xi = ZSqrt2(int_t(pow(2, temp_k)), 0) -
                                (candidate.conj() * candidate).to_zsqrt2();
                    ZOmega answer(0);
                    solution_found = diophantine_solver(answer, xi);
                    if (solution_found) {
                        return RzApproximation(candidate, answer, temp_k, theta,
                                               eps);
                    }
                }
            }

        for (auto alpha_soln : shifted_alpha_solns)
            for (auto beta_soln : shifted_beta_solns) {
                ZOmega candidate = G * ZOmega(alpha_soln, beta_soln, 1);
                if (((candidate.real() / scaleA) * z[0] +
                     (candidate.imag() / scaleA) * z[1]) >
                    1 - (eps * eps / 2)) {
                    int_t temp_k = k;
                    while (candidate.is_reducible()) {
                        temp_k -= 1;
                        candidate = candidate.reduce();
                    }

                    ZSqrt2 xi = ZSqrt2(int_t(pow(2, temp_k)), 0) -
                                (candidate.conj() * candidate).to_zsqrt2();
                    ZOmega answer(0);
                    solution_found = diophantine_solver(answer, xi);
                    if (solution_found) {
                        return RzApproximation(candidate, answer, temp_k, theta,
                                               eps);
                    }
                }
            }

        k++;
        bboxA.rescale(1 / scaleA, 1 / scaleA);
        bboxB.rescale(1 / scaleB, 1 / scaleB);
    }
    return RzApproximation();
}

} // namespace grid_synth
} // namespace staq

#endif // RZ_APPROXIMATION_HPP
