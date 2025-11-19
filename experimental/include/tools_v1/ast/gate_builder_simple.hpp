#ifndef GATE_BUILDER_SIMPLE_HPP_
#define GATE_BUILDER_SIMPLE_HPP_

#include "base.hpp"
#include "control_gate.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include <cassert>
#include <list>
#include <stdexcept>
#include <vector>

namespace tools_v1::ast {

namespace PrimitiveGate {
enum Type {
  PAULI_STRING,
  CONTROL,
  MULTI_CONTROL,
  EXP_PAULI,
  DECLARED_GATE,
  CNOT,
  HADAMARD
};

inline PauliType pauli_from_string(const std::string &str) {
  if (str == "X" || str == "x")
    return PauliType::X;
  if (str == "Y" || str == "y")
    return PauliType::Y;
  if (str == "Z" || str == "z")
    return PauliType::Z;
  if (str == "I" || str == "i")
    return PauliType::I;
  throw std::invalid_argument("Invalid Pauli string: " + str);
}
} // namespace PrimitiveGate

class GateBuilder {
private:
  std::vector<ptr<Gate>> gates;
  std::list<ptr<Stmt>> stmts;
  PrimitiveGate::Type current_type;
  std::vector<VarAccess> qubits;
  std::vector<PauliType> paulis;
  ptr<Gate> target_gate;
  ptr<Expr> angle;
  tools_v1::parser::Position pos;

  // For control gates
  bool building_nested_gate = false;

  // For MultiControlGate: track which qubits go to ctrl1 vs ctrl2
  bool after_separator = false;
  std::vector<VarAccess> ctrl1_qubits;
  std::vector<VarAccess> ctrl2_qubits;

  void build_and_add_gate() {
    ptr<Gate> built_gate;

    switch (current_type) {
    case PrimitiveGate::PAULI_STRING:
      if (qubits.size() != paulis.size()) {
        throw std::runtime_error(
            "PauliString requires equal number of qubits and Pauli operators");
      }
      built_gate =
          PauliString::create(pos, std::move(qubits), std::move(paulis));
      break;
    case PrimitiveGate::EXP_PAULI:
      if (qubits.size() != paulis.size()) {
        throw std::runtime_error(
            "ExpPauli requires equal number of qubits and Pauli operators");
      }
      if (!angle) {
        throw std::runtime_error("ExpPauli requires an angle expression");
      }
      built_gate = ExpPauli::create(pos, std::move(angle), std::move(qubits),
                                    std::move(paulis));
      break;
    case PrimitiveGate::CNOT:
      if (qubits.size() != 2) {
        throw std::runtime_error(
            "ControlGate requires exactly one control qubit");
      }
      built_gate =
          CNOTGate::create(pos, std::move(qubits[0]), std::move(qubits[1]));
      break;
    case PrimitiveGate::CONTROL:
      if (qubits.size() != 1) {
        throw std::runtime_error(
            "ControlGate requires exactly one control qubit");
      }
      if (!target_gate) {
        throw std::runtime_error("ControlGate requires a target gate");
      }
      built_gate = ControlGate::create(pos, std::move(qubits[0]),
                                       std::move(target_gate));
      break;
    case PrimitiveGate::MULTI_CONTROL:
      if (!target_gate) {
        throw std::runtime_error("MultiControlGate requires a target gate");
      }
      // For MultiControlGate, use the separated ctrl1 and ctrl2 qubits
      built_gate = MultiControlGate::create(pos, std::move(ctrl1_qubits),
                                            std::move(ctrl2_qubits),
                                            std::move(target_gate));
      break;
    default:
      throw std::runtime_error("Gate type not yet implemented");
    }

    if (built_gate) {
      gates.push_back(std::move(built_gate));
    }

    // Reset for next gate
    qubits.clear();
    paulis.clear();
    target_gate.reset();
    angle.reset();
    building_nested_gate = false;
    after_separator = false;
    ctrl1_qubits.clear();
    ctrl2_qubits.clear();
  }

public:
  GateBuilder() : pos() {}
  // GateBuilder(PrimitiveGate::Type type) : pos(), current_type(type) {}

  GateBuilder &operator+=(PrimitiveGate::Type gate_type) {
    if (!qubits.empty() || !paulis.empty() || target_gate) {
      build_and_add_gate();
    }
    current_type = gate_type;
    return *this;
  }

  GateBuilder &operator[](PrimitiveGate::Type gate_type) {
    assert(qubits.empty() && paulis.empty() && after_separator == false);
    current_type = gate_type;
    return *this;
  }

  // ExpPauli ONLY
  GateBuilder &operator,(const std::string &str) {
    // FIX: uncomment in the future
    // assert(current_type == PrimitiveGate::EXP_PAULI);
    if (str == "pi/4" || str == "π/4") {
      angle = BExpr::create(pos, PiExpr::create(pos), BinaryOp::Divide, RealExpr::create(pos, 4.0));
    } else if (str == "pi/2" || str == "π/2") {
      angle = BExpr::create(pos, PiExpr::create(pos), BinaryOp::Divide, RealExpr::create(pos, 2.0));
    } else if (str == "pi" || str == "π") {
      angle = PiExpr::create(pos);
    } else {
      paulis.push_back(PrimitiveGate::pauli_from_string(str));
    }
    return *this;
  }

  // ExpPauli ONLY
  GateBuilder &operator,(double angle_value) {
    // FIX: uncomment in the future
    // assert(current_type == PrimitiveGate::EXP_PAULI);
    angle = RealExpr::create(pos, angle_value);
    return *this;
  }

  // Pauli or ExpPauli
  GateBuilder &operator,(PauliType pauli) {
    paulis.push_back(pauli);
    return *this;
  }

  // original implementation, kept for backward compatibility
  GateBuilder &operator,(ptr<Gate> gate) {
    if (current_type == PrimitiveGate::CONTROL ||
        current_type == PrimitiveGate::MULTI_CONTROL) {
      target_gate = std::move(gate);
    }
    return *this;
  }


  GateBuilder &operator*(PauliType pauli) {
    assert(current_type == PrimitiveGate::PAULI_STRING || 
           current_type == PrimitiveGate::EXP_PAULI);
    paulis.push_back(pauli);
    return *this;
  }


  GateBuilder &operator*(const VarAccess &qubit) {
    // assert(current_type == PrimitiveGate::CONTROL ||
    //        current_type == PrimitiveGate::MULTI_CONTROL);
    if (current_type == PrimitiveGate::MULTI_CONTROL) {
      if (after_separator) {
        ctrl2_qubits.push_back(qubit);
      } else {
        ctrl1_qubits.push_back(qubit);
      }
    } else {
      qubits.push_back(qubit);
    }
    return *this;
  }

  GateBuilder &operator*(ptr<Gate> gate) {
    assert(current_type == PrimitiveGate::CONTROL ||
           current_type == PrimitiveGate::MULTI_CONTROL);
    target_gate = std::move(gate);
    return *this;
  }

  GateBuilder &operator%(ptr<Gate> gate) {
    assert(current_type == PrimitiveGate::CONTROL ||
           current_type == PrimitiveGate::MULTI_CONTROL);
    target_gate = std::move(gate);
    return *this;
  }

  GateBuilder &operator/(const VarAccess &qubit) {
    assert(current_type == PrimitiveGate::MULTI_CONTROL);
    assert(after_separator == false);
    after_separator = true;
    ctrl2_qubits.push_back(qubit);
    return *this;
  }

  GateBuilder &operator/(ptr<Gate> gate) {
    assert(current_type == PrimitiveGate::MULTI_CONTROL);
    assert(after_separator == false);
    after_separator = true;
    target_gate = std::move(gate);
    return *this;
  }

  GateBuilder &separate() {
    if (current_type == PrimitiveGate::MULTI_CONTROL) {
      after_separator = true;
    }
    return *this;
  }

  std::vector<ptr<Gate>> submit() {
    // Finish current gate if any
    if (!qubits.empty() || !paulis.empty() || target_gate) {
      build_and_add_gate();
    }
    return std::move(gates);
  }

  std::list<ptr<Stmt>> submit_list() {
    auto vec_gates = submit();
    std::list<ptr<Stmt>> lst;
    std::transform(vec_gates.begin(), vec_gates.end(), std::back_inserter(lst),
                   [](const ptr<Gate> &gate) -> ptr<Stmt> {
                     return object::clone(*gate);
                   });
    return lst;
  }
};

inline GateBuilder gates() { return GateBuilder{}; }

} // namespace tools_v1::ast

#endif
