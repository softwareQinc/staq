#ifndef GATE_BUILDER_HPP_
#define GATE_BUILDER_HPP_

#include "base.hpp"
#include "expr.hpp"
#include "stmt.hpp"
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

PauliType pauli_from_string(const std::string &str);
} // namespace PrimitiveGate

template <typename T> class GateBuilder {
private:
  T data;
  PrimitiveGate::Type current_type;
  std::vector<VarAccess> qubits;
  std::vector<PauliType> paulis;
  ptr<Gate> target_gate;
  ptr<Expr> angle;
  tools_v1::parser::Position pos;

  ptr<Gate> build_pauli_string();
  ptr<Gate> build_control_gate();
  ptr<Gate> build_exp_pauli();

public:
  GateBuilder() : pos() {}
  GateBuilder<T> &operator+=(PrimitiveGate::Type gate_type);
  GateBuilder<T> &operator,(const VarAccess &qubit);
  GateBuilder<T> &operator,(const std::string &qubit_name);
  GateBuilder<T> &operator,(PauliType pauli);
  GateBuilder<T> &operator,(double angle_value);
  GateBuilder<T> &operator,(ptr<Expr> expr);
  GateBuilder<T> &operator,(PrimitiveGate::Type nested_gate_type);
  T submit();
  void reset();
};

// Specialization for vector of gates
class GateVectorBuilder : public GateBuilder<std::vector<ptr<Gate>>> {
private:
  std::vector<ptr<Gate>> gates;

public:
  GateVectorBuilder &operator+=(PrimitiveGate::Type gate_type);
  GateVectorBuilder &operator,(PrimitiveGate::Type next_gate_type);
  std::vector<ptr<Gate>> submit();
};

GateVectorBuilder gates();
GateBuilder<ptr<Gate>> gate();

} // namespace tools_v1::ast

#endif
