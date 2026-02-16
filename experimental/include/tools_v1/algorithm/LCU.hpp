#ifndef LCU_HPP_
#define LCU_HPP_

#include <cassert>
#include <circuit_dagger.hpp>
#include <cmath>
#include <complex>
#include <functional>
#include <numbers>
#include <tools_v1/algorithm/Utils.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>
#include <vector>

namespace tools_v1::algorithm {

using namespace tools_v1::tools;

inline circuit lcu_prepare(const std::vector<qbit> &ancillas) {
  circuit prep_circuit;
  tools_v1::parser::Position pos;
  const int num_ancillas = ancillas.size();

  for (int L = 0; L < num_ancillas; ++L) {
    auto h =
        ast::DeclaredGate::create(pos, "h", {}, {ancillas[L].to_va()});
    prep_circuit.push_back(std::move(h));
  }

  return prep_circuit;
}

inline circuit lcu_prepare(const std::vector<double> &coefficients,
                           const std::vector<qbit> &ancillas) {
  circuit prep_circuit;
  tools_v1::parser::Position pos;
  const int num_ancillas = ancillas.size();
  const int num_coeffs = coefficients.size();

  assert(num_coeffs == (1 << num_ancillas));

  std::vector<double> cs;
  cs.reserve(coefficients.size());
  std::partial_sum(coefficients.begin(), coefficients.end(),
                   std::back_inserter(cs));

  auto S = [&cs, &num_ancillas](int L, int k) -> double {
    assert(L <= num_ancillas);
    assert(k <= (1 << L) - 1);
    int ind_max = (k + 1) * (1 << (num_ancillas - L)) - 1;
    int ind_min = k * (1 << (num_ancillas - L)) - 1;
    assert(ind_max < cs.size());
    if (k == 0)
      return cs[ind_max];
    else
      return cs[ind_max] - cs[ind_min];
  };

  auto mu = [&cs, &num_ancillas, &S](int L, int k) -> double {
    assert(L <= num_ancillas - 1);
    assert(k <= (1 << L) - 1);
    return std::sqrt(S(L + 1, 2 * k) / S(L, k));
  };

  for (int L = 1; L < num_ancillas; ++L) {
    for (int k = 0; k < (1 << L); ++k) {

      std::vector<int> controls_1;
      std::vector<int> controls_0;

      for (int j = 0; j < L; ++j) {
        if (k & (1 << (L - 1 - j))) {
          controls_1.push_back(j);
        } else {
          controls_0.push_back(j);
        }
      }

      std::vector<ast::VarAccess> ctrl1_qubits;
      std::vector<ast::VarAccess> ctrl2_qubits;

      for (int idx : controls_1) {
        ctrl1_qubits.push_back(ancillas[idx].to_va());
      }

      for (int idx : controls_0) {
        ctrl2_qubits.push_back(ancillas[idx].to_va());
      }

      auto tmp = ry_gate(-std::acos(mu(L, k)), ancillas[L]);

      auto g = ast::MultiControlGate::create(pos, std::move(ctrl1_qubits),
                                             std::move(ctrl2_qubits),
                                             std::move(tmp));
      prep_circuit.push_back(std::move(g));
    }
  }

  return prep_circuit;
}

inline circuit lcu_select(const std::vector<qbit> &ancillas,
                          const std::vector<circuit> &unitaries) {
  circuit c;
  tools_v1::parser::Position pos;

  int N = unitaries.size();
  int A = ancillas.size();
  assert(N == (1 << A));

  auto it = unitaries.begin();
  for (int i = 0; i < N; ++i, ++it) {
    std::vector<int> controls_1;
    std::vector<int> controls_0;

    for (int j = 0; j < ancillas.size(); ++j) {
      if (i & (1 << j)) {
        controls_1.push_back(j);
      } else {
        controls_0.push_back(j);
      }
    }

    std::vector<ast::VarAccess> ctrl1_qubits;
    std::vector<ast::VarAccess> ctrl2_qubits;

    for (int idx : controls_1) {
      ctrl1_qubits.push_back(ancillas[idx].to_va());
    }

    for (int idx : controls_0) {
      ctrl2_qubits.push_back(ancillas[idx].to_va());
    }

    for (auto &v : *it) {
      ast::ptr<ast::Stmt> tmp = ast::object::clone(*v);
      auto g_s = stmt_to_gate(*tmp);
      auto g = ast::MultiControlGate::create(pos, std::move(ctrl1_qubits),
                                             std::move(ctrl2_qubits),
                                             std::move(g_s));
      c.push_back(std::move(g));
    }
  }

  return c;
}

inline circuit lcu_select(const std::vector<qbit> &ancillas,
                          const std::vector<ast::ptr<ast::Gate>> &unitaries) {
  circuit c;
  tools_v1::parser::Position pos;

  int N = unitaries.size();
  int A = ancillas.size();
  assert(N == (1 << A));

  auto it = unitaries.begin();
  for (int i = 0; i < N; ++i, ++it) {
    std::vector<int> controls_1;
    std::vector<int> controls_0;

    for (int j = 0; j < ancillas.size(); ++j) {
      if (i & (1 << j)) {
        controls_1.push_back(j);
      } else {
        controls_0.push_back(j);
      }
    }

    std::vector<ast::VarAccess> ctrl1_qubits;
    std::vector<ast::VarAccess> ctrl2_qubits;

    for (int idx : controls_1) {
      ctrl1_qubits.push_back(ancillas[idx].to_va());
    }

    for (int idx : controls_0) {
      ctrl2_qubits.push_back(ancillas[idx].to_va());
    }

    // FIX: what's going on here? why does this work?
    auto tmp = ast::object::clone(**it);

    auto g = ast::MultiControlGate::create(
        pos, std::move(ctrl1_qubits), std::move(ctrl2_qubits), std::move(tmp));
    c.push_back(std::move(g));
  }

  return c;
}

inline circuit LCU(const std::vector<double> &coefficients,
                   const std::vector<qbit> &ancilla_qubits,
                   const std::vector<circuit> &unitaries) {
  circuit lcu_circuit;
  circuit prep = lcu_prepare(coefficients, ancilla_qubits);
  circuit sel = lcu_select(ancilla_qubits, unitaries);

  for (const auto &x : ancilla_qubits)
    lcu_circuit.save_ancilla(x);

  for (const auto &gate : prep) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  for (const auto &gate : sel) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  // circuit prep_dagger = dagger_circuit(prep);
  auto prep_dagger = ast::circuit_dagger(prep);
  for (const auto &gate : prep_dagger) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  return lcu_circuit;
}

inline circuit LCU(const std::vector<double> &coefficients,
                   const std::vector<qbit> &ancilla_qubits,
                   const std::vector<ast::ptr<ast::Gate>> &unitaries) {
  circuit lcu_circuit;
  circuit prep = lcu_prepare(coefficients, ancilla_qubits);
  circuit sel = lcu_select(ancilla_qubits, unitaries);

  for (const auto &x : ancilla_qubits)
    lcu_circuit.save_ancilla(x);

  for (const auto &gate : prep) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  for (const auto &gate : sel) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  // circuit prep_dagger = dagger_circuit(prep);
  auto prep_dagger = ast::circuit_dagger(prep);
  for (const auto &gate : prep_dagger) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  return lcu_circuit;
}

inline circuit LCU(const std::vector<ast::ptr<ast::Gate>> &unitaries, ANC_MEM& anc_mem) {
  circuit lcu_circuit;
  std::vector<qbit> ancilla_qubits;
  const int NA = std::log2(unitaries.size());
  ancilla_qubits.reserve(NA);
  for (int i = 0; i < NA; ++i)
    ancilla_qubits.emplace_back(anc_mem.generate_ancilla("LCU"));

  circuit prep = lcu_prepare(ancilla_qubits);
  circuit sel = lcu_select(ancilla_qubits, unitaries);

  for (const auto &x : ancilla_qubits)
    lcu_circuit.save_ancilla(x);

  for (const auto &gate : prep) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  for (const auto &gate : sel) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  // circuit prep_dagger = dagger_circuit(prep);
  auto prep_dagger = ast::circuit_dagger(prep);
  for (const auto &gate : prep_dagger) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  return lcu_circuit;
}

inline circuit LCU(const std::vector<circuit> &unitaries, ANC_MEM& anc_mem) {
  circuit lcu_circuit;
  std::vector<qbit> ancilla_qubits;
  const int NA = std::log2(unitaries.size());
  ancilla_qubits.reserve(NA);
  for (int i = 0; i < NA; ++i)
    ancilla_qubits.emplace_back(anc_mem.generate_ancilla("LCU"));

  circuit prep = lcu_prepare(ancilla_qubits);
  circuit sel = lcu_select(ancilla_qubits, unitaries);

  for (const auto &x : ancilla_qubits)
    lcu_circuit.save_ancilla(x);

  for (const auto &gate : prep) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  for (const auto &gate : sel) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  // circuit prep_dagger = dagger_circuit(prep);
  auto prep_dagger = ast::circuit_dagger(prep);
  for (const auto &gate : prep_dagger) {
    lcu_circuit.push_back(tools_v1::ast::object::clone(*gate));
  }

  return lcu_circuit;
}

inline circuit LCU_two_unitaries(std::complex<double> c0,
                                 std::complex<double> c1, const circuit &U0,
                                 const circuit &U1, const qbit &ancilla) {
  circuit lcu_circuit;
  tools_v1::parser::Position pos;
  // save ancillas
  lcu_circuit.save_ancilla(ancilla);
  for (auto it = U0.ancilla_begin(); it != U0.ancilla_end(); ++it)
    lcu_circuit.save_ancilla(**it);
  for (auto it = U1.ancilla_begin(); it != U1.ancilla_end(); ++it)
    lcu_circuit.save_ancilla(**it);

  double theta =
      2.0 * std::acos(std::sqrt(std::abs(c0) / (std::abs(c0) + std::abs(c1))));
  double mu = std::arg(c0) - std::arg(c1);

  lcu_circuit.push_back(ry_gate(theta, ancilla));
  lcu_circuit.push_back(rz_gate(mu, ancilla));

  std::vector<tools_v1::ast::VarAccess> ctrl1_qubits,
      ctrl2_qubits = {ancilla.to_va()};
  for (const auto &stmt : U0) {
    GateConverter conv;
    stmt->accept(conv);
    auto _ctr1 = ctrl1_qubits;
    auto _ctr2 = ctrl2_qubits;
    auto g =
        ast::MultiControlGate::create(pos, std::move(_ctr1), std::move(_ctr2),
                                      std::move(conv.converted_gate));
    lcu_circuit.push_back(std::move(g));
  }

  ctrl2_qubits.clear();
  ctrl1_qubits = {ancilla.to_va()};
  for (const auto &stmt : U1) {
    GateConverter conv;
    stmt->accept(conv);
    auto _ctr1 = ctrl1_qubits;
    auto _ctr2 = ctrl2_qubits;
    auto g =
        ast::MultiControlGate::create(pos, std::move(_ctr1), std::move(_ctr2),
                                      std::move(conv.converted_gate));
    lcu_circuit.push_back(std::move(g));
  }

  lcu_circuit.push_back(ry_gate(-theta, ancilla));
  return lcu_circuit;
}

inline circuit LCU_two_unitaries(std::complex<double> c0,
                                 std::complex<double> c1, const circuit &U0,
                                 const circuit &U1, ANC_MEM& anc_mem) {
  qbit ancilla = anc_mem.generate_ancilla("LCU_two");
  return LCU_two_unitaries(c0, c1, U0, U1, ancilla);
}

} // namespace tools_v1::algorithm

#endif
