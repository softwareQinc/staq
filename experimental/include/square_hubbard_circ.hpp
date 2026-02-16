#ifndef SQUARE_HUBBARD_CIRC_HPP_
#define SQUARE_HUBBARD_CIRC_HPP_

#include <cmath>
#include <numbers>
#include <square_hubbard_config.hpp>
#include <tools_v1/algorithm/LCU.hpp>
#include <tools_v1/tools/staq_builder.hpp>
#include <vector>

// using qbit = tools_v1::tools::qbit;
// using circuit = tools_v1::tools::circuit;

class square_hubbard_circ {
private:
  std::vector<tools_v1::tools::qbit> _data_qubits;
  square_hubbard_config _config;

public:
  square_hubbard_circ(int n, unsigned int L, double t, double U)
      : _data_qubits(n), _config(L, t, U) {}

  tools_v1::tools::circuit creation_op(int i);
  tools_v1::tools::circuit annihilation_op(int i);

  tools_v1::tools::circuit kinetic_op();
  tools_v1::tools::circuit interaction_op();
  std::pair<int, int> n_to_nx_ny(int n);
  int nx_ny_to_n(int nx, int ny);

  std::vector<double> generate_kinetic_coefficients();
  std::vector<tools_v1::tools::circuit> generate_kinetic_unitaries();
};

#endif
