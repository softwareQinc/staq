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

namespace staq{
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
        z_ = cplx_t(std::cos(theta.get_d()), std::sin(theta.get_d()));
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
    //int_t k = 3 * int_t(log(real_t(1) / eps) / log(2)) / 2;
    int_t k = 0;
    int_t max_k = 1000;
    bool solution_found = false;
    zomega_vec_t scaled_candidates;
    vec_t z{std::cos(theta.get_d()), std::sin(theta.get_d())};
    Ellipse eps_region(theta, eps);
    Ellipse disk(0, 0, 1, 1, 0);

    state_t state{eps_region, disk};

    SpecialGridOperator G = optimize_skew(state);
    real_t scale;
    while ((not solution_found) && k < max_k) {
        if (k % 2 == 0)
            scale = pow(2, k / 2);
        else
            scale = pow(2, (k - 1) / 2) * SQRT2;
        state[0].rescale(scale);
        state[1].rescale(-scale);
        scaled_candidates = twoD_grid_solver_ellipse(state, tol);
        for (auto scaled_candidate : scaled_candidates) {
            auto candidate = G * scaled_candidate;
            if (((candidate.real() / scale) * z[0] +
                 (candidate.imag() / scale) * z[1]) > 1 - (eps * eps / 2)) {
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
        state[0].rescale(1 / scale);
        state[1].rescale(-1 / scale);
    }
    return RzApproximation();
}

} // namespace grid_synth
} // namespace staq

#endif // RZ_APPROXIMATION_HPP
