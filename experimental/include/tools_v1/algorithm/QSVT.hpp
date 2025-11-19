#pragma once

#include <tools_v1/algorithm/Utils.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>
#include <vector>
#include <circuit_dagger.hpp>

namespace tools_v1::algorithm {

using namespace tools_v1::tools;

// Create a controlled Z rotation gate
inline tools_v1::ast::ptr<tools_v1::ast::Gate>
controlled_rz_gate(double angle, const qbit &control, const qbit &target) {
  tools_v1::parser::Position pos;
  auto angle_expr = tools_v1::ast::RealExpr::create(pos, angle);

  // Create controlled Rz gate
  auto target_gate = rz(target, std::move(angle_expr));

  // Create controlled version
  auto control_qubit = control.to_va();

  return tools_v1::ast::ControlGate::create(pos, std::move(control_qubit),
                                            std::move(target_gate));
}

// Create a multi-controlled Z rotation gate
inline tools_v1::ast::ptr<tools_v1::ast::Gate>
multi_controlled_rz_gate(double angle, const std::vector<qbit> &controls,
                         const qbit &target) {
  tools_v1::parser::Position pos;
  auto angle_expr = tools_v1::ast::RealExpr::create(pos, angle);

  // Create the target Z rotation gate
  auto target_gate = rz(target, std::move(angle_expr));

  // Create multi-controlled version
  std::vector<tools_v1::ast::VarAccess> control_qubits;
  for (const auto &control : controls) {
    control_qubits.push_back(control.to_va());
  }

  return tools_v1::ast::MultiControlGate::create(
      pos, std::move(control_qubits), std::vector<tools_v1::ast::VarAccess>{},
      std::move(target_gate));
}

// Quantum Singular Value Transform (QSVT) implementation
inline circuit QSVT(const std::vector<double> &phi, const circuit &u,
                    const qbit &qsvt_ancilla) {
  circuit qsvt_circuit;
  assert(phi.size() % 2 == 1);
  const int d = (phi.size() - 1) / 2;
  tools_v1::parser::Position pos;

  // Prepare ancillas
  std::vector<std::unique_ptr<qbit>> all_ancillas;
  for (auto it = u.ancilla_begin(); it != u.ancilla_end(); ++it) {
    all_ancillas.push_back(std::make_unique<qbit>(**it));
  }
  auto convert_to_vec_varaccess =
      [](const std::vector<std::unique_ptr<qbit>> &v)
      -> std::vector<ast::VarAccess> {
    tools_v1::parser::Position pos;
    std::vector<ast::VarAccess> qubits;
    qubits.reserve(v.size());
    for (auto &q : v) {
      qubits.emplace_back(q->to_va());
    }
    return qubits;
  };

  // Prepare helper functions for creating multi-controlled gates
  auto create_x_gate = [&qsvt_ancilla, &pos]() {
    return tools_v1::ast::DeclaredGate::create(pos, "x", {},
                                               {qsvt_ancilla.to_va()});
  };
  auto create_mct_gate = [&all_ancillas, &pos, &convert_to_vec_varaccess,
                          &create_x_gate]() {
    auto x = create_x_gate();
    return tools_v1::ast::MultiControlGate::create(
        pos, {}, convert_to_vec_varaccess(all_ancillas), std::move(x));
  };
  ast::ptr<ast::MultiControlGate> qsvt_mct_gate;
  ast::ptr<ast::DeclaredGate> qsvt_rz_gate;

  // Initial Hadamard on QSVT ancilla
  qsvt_circuit.push_back(hadamard(qsvt_ancilla));

  // Iterate through all phases (2d+1 total phases)
  qsvt_circuit.push_back(std::move(create_mct_gate()));
  qsvt_circuit.push_back(std::move(rz_gate(phi.back(), qsvt_ancilla)));
  qsvt_circuit.push_back(std::move(create_mct_gate()));

  // prepare helper functions to add unitaries

  auto add_u = [&qsvt_circuit, &u](){
    for(auto it = u.begin(); it != u.end(); ++it){
      qsvt_circuit.push_back(ast::object::clone(**it));
    }
  };
  auto u_dag = ast::circuit_dagger(u);
  auto add_u_dag = [&qsvt_circuit, &u_dag](){
    for(auto it = u_dag.begin(); it != u_dag.end(); ++it){
      qsvt_circuit.push_back(ast::object::clone(**it));
    }
  };

  for (int k = 2 * d; k >= 0; k -= 2) {

    add_u();

    qsvt_circuit.push_back(std::move(create_mct_gate()));
    qsvt_circuit.push_back(std::move(rz_gate(phi[k-1], qsvt_ancilla)));
    qsvt_circuit.push_back(std::move(create_mct_gate()));

    add_u_dag();

    qsvt_circuit.push_back(std::move(create_mct_gate()));
    qsvt_circuit.push_back(std::move(rz_gate(phi[k-2], qsvt_ancilla)));
    qsvt_circuit.push_back(std::move(create_mct_gate()));
  }

  qsvt_circuit.push_back(hadamard(qsvt_ancilla));

  return qsvt_circuit;
}

inline circuit QSVT(const std::vector<double> &phi, const circuit &u, ANC_MEM& anc_mem) {
  qbit qsvt_ancilla = anc_mem.generate_ancilla("QSVT");
  return QSVT(phi, u, qsvt_ancilla);
}

} // namespace tools_v1::algorithm
