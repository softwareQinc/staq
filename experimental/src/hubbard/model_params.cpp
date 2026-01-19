#include <hubbard/model_params.hpp>

namespace hubbard {

ModelParams::ModelParams(unsigned ell_value, double t_value, double U_value,
                         double E0_value, std::complex<double> z_value)
    : ell(ell_value), L(1u << ell_value), t(t_value), U(U_value), E0(E0_value),
      z(z_value) {}

ModelParams ModelParams::real_space_defaults() {
  return ModelParams(7u, 1.0, 4.0, 3.0, {3.0, 4.0});
}

int ModelParams::num_fermions() const {
  return 2 * static_cast<int>(L) * static_cast<int>(L);
}

} // namespace hubbard
