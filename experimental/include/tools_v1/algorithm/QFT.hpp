#pragma once

#include <tools_v1/tools/staq_builder.hpp>
#include <vector>
#include <numbers>

namespace tools_v1::algorithm {

using namespace tools_v1::tools;

// Create a phase rotation gate R_k = diag(1, exp(2*pi*i/2^k))
inline tools_v1::ast::ptr<tools_v1::ast::Gate> phase_rotation(int k,
                                                              const qbit &target) {
  tools_v1::parser::Position pos;
  double angle = 2.0 * std::numbers::pi / std::pow(2.0, k);

  // Create a U gate with the phase rotation
  auto angle_expr = tools_v1::ast::RealExpr::create(pos, angle);
  std::vector<tools_v1::ast::VarAccess> target_qubits = {
      target.to_var_access()};

  return tools_v1::ast::PhaseGate::create(pos, std::move(angle_expr),
                                          std::move(target_qubits));
}

// Create a controlled phase rotation gate
inline tools_v1::ast::ptr<tools_v1::ast::Gate>
controlled_phase_rotation(int k, const qbit &control, const qbit &target) {
  tools_v1::parser::Position pos;

  // Create the target phase rotation gate
  auto target_gate = phase_rotation(k, target);

  // Create controlled version
  auto control_qubit = control.to_var_access();

  return tools_v1::ast::ControlGate::create(pos, std::move(control_qubit),
                                            std::move(target_gate));
}

// Quantum Fourier Transform implementation
inline circuit QFT(std::vector<qbit> qubits) {
  circuit qft_circuit;
  int n = qubits.size();

  for (int i = 0; i < n; ++i) {
    // Add Hadamard gate on current qubit
    qft_circuit.push_back(hadamard(qubits[i]));

    // Add controlled phase rotations
    for (int j = i + 1; j < n; ++j) {
      int k = j - i + 1; // R_k where k = j-i+1
      auto controlled_phase =
          controlled_phase_rotation(k, qubits[j], qubits[i]);
      qft_circuit.push_back(std::move(controlled_phase));
    }
  }

  return qft_circuit;
}

// Inverse Quantum Fourier Transform
inline circuit inverse_QFT(std::vector<qbit> qubits) {
  circuit inv_qft_circuit;
  int n = qubits.size();

  for (int i = n - 1; i >= 0; --i) {
    // Add controlled phase rotations (in reverse order)
    for (int j = n - 1; j > i; --j) {
      int k = j - i + 1; // R_k where k = j-i+1
      auto controlled_phase =
          controlled_phase_rotation(-k, qubits[j], qubits[i]);
      inv_qft_circuit.push_back(std::move(controlled_phase));
    }

    // Add Hadamard gate on current qubit
    inv_qft_circuit.push_back(hadamard(qubits[i]));
  }

  return inv_qft_circuit;
}

} // namespace tools_v1::algorithm