#ifndef HUBBARD_MODEL_PARAMS_HPP_
#define HUBBARD_MODEL_PARAMS_HPP_

#include <complex>

namespace hubbard {

struct ModelParams {
  unsigned ell = 0;
  unsigned L = 0;
  double t = 0.0;
  double U = 0.0;
  double E0 = 0.0;
  std::complex<double> z{0.0, 0.0};

  ModelParams() = default;
  ModelParams(unsigned ell_value, double t_value, double U_value,
              double E0_value, std::complex<double> z_value);

  static ModelParams real_space_defaults();

  int num_fermions() const;
};

} // namespace hubbard

#endif // HUBBARD_MODEL_PARAMS_HPP_
