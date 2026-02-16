#ifndef TOOLS_V1_STAQBUILDER_HPP_
#define TOOLS_V1_STAQBUILDER_HPP_

#include "node_conversion.hpp"
#include "tools_v1/ast/ast.hpp"
#include "tools_v1/ast/decl.hpp"
#include "tools_v1/ast/expr.hpp"
#include "tools_v1/ast/gate_builder_simple.hpp"
#include "tools_v1/ast/program.hpp"
#include "tools_v1/ast/stmt.hpp"
#include "tools_v1/ast/visitor.hpp"
#include "tools_v1/parser/parser.hpp"
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <functional>
#include "node_conversion.hpp"
#include <iterator>

namespace tools_v1::tools {

class pauli_literal {
private:
  std::string type_;
  int qubit_index_;

public:
  pauli_literal(const std::string &type, int qubit_index)
      : type_(type), qubit_index_(qubit_index) {}

  const std::string &type() const { return type_; }
  int qubit_index() const { return qubit_index_; }
};

inline pauli_literal operator""_x(unsigned long long int x) noexcept {
  return pauli_literal("X", static_cast<int>(x));
}

inline pauli_literal operator""_y(unsigned long long int x) noexcept {
  return pauli_literal("Y", static_cast<int>(x));
}

inline pauli_literal operator""_z(unsigned long long int x) noexcept {
  return pauli_literal("Z", static_cast<int>(x));
}

class qbit {
private:
  std::string register_name_;
  int index_;

public:
  qbit() : register_name_("q"), index_(0) {}
  qbit(const std::string &register_name, int index)
      : register_name_(register_name), index_(index) {}

  qbit(int index) : register_name_("q"), index_(index) {}

  const std::string &register_name() const { return register_name_; }
  int index() const { return index_; }

  ast::VarAccess to_va() const {
    tools_v1::parser::Position pos;
    return tools_v1::ast::VarAccess(pos, register_name_, index_);
  }

  pauli_literal x() const { return pauli_literal("X", index_); }
  pauli_literal y() const { return pauli_literal("Y", index_); }
  pauli_literal z() const { return pauli_literal("Z", index_); }

  // Comparison operator for std::set
  bool operator<(const qbit& other) const {
    if (register_name_ != other.register_name_) {
      return register_name_ < other.register_name_;
    }
    return index_ < other.index_;
  }
};

class circuit {
private:
  std::vector<ast::ptr<ast::Stmt>> _gates;
  std::set<ast::ptr<qbit>> _ancilla;
  std::set<ast::ptr<qbit>> _data;

public:
  using iterator = std::vector<ast::ptr<ast::Stmt>>::iterator;
  using iterator_gate = std::vector<ast::ptr<ast::Gate>>::iterator;
  using const_iterator = std::vector<ast::ptr<ast::Stmt>>::const_iterator;
  using reverse_iterator = std::vector<ast::ptr<ast::Stmt>>::reverse_iterator;
  using const_reverse_iterator =
      std::vector<ast::ptr<ast::Stmt>>::const_reverse_iterator;

  circuit() = default;

  // FIX: What are these for?
  circuit(const circuit &) = delete;
  circuit &operator=(const circuit &) = delete;
  circuit(circuit &&) = default;
  circuit &operator=(circuit &&) = default;

  void push_back(ast::ptr<ast::Stmt> gate) {
    _gates.push_back(std::move(gate));
  }

  void reserve(size_t n) { _gates.reserve(n); }

  size_t size() const { return _gates.size(); }

  bool empty() const { return _gates.empty(); }

  iterator begin() { return _gates.begin(); }
  iterator end() { return _gates.end(); }
  const_iterator begin() const { return _gates.begin(); }
  const_iterator end() const { return _gates.end(); }
  reverse_iterator rbegin() { return _gates.rbegin(); }
  reverse_iterator rend() { return _gates.rend(); }
  const_reverse_iterator rbegin() const { return _gates.rbegin(); }
  const_reverse_iterator rend() const { return _gates.rend(); }

  void splice(iterator pos, iterator first, iterator last) {
    auto pos_idx = std::distance(_gates.begin(), pos);
    auto first_idx = std::distance(_gates.begin(), first);
    auto last_idx = std::distance(_gates.begin(), last);

    std::vector<ast::ptr<ast::Stmt>> temp;
    temp.reserve(last_idx - first_idx);

    for (auto it = first; it != last; ++it) {
      temp.push_back(std::move(*it));
    }

    _gates.insert(_gates.begin() + pos_idx,
                  std::make_move_iterator(temp.begin()),
                  std::make_move_iterator(temp.end()));
  }

  void operator+=(circuit c) { this->splice(this->end(), c.begin(), c.end()); }

  inline std::list<tools_v1::ast::ptr<tools_v1::ast::Stmt>> body_list() const {
    using ptrStmt = tools_v1::ast::ptr<tools_v1::ast::Stmt>;
    std::list<ptrStmt> lst;
    std::transform(_gates.begin(), _gates.end(), std::back_inserter(lst),
                   [](const ptrStmt &gate) -> ptrStmt {
                     return tools_v1::ast::object::clone(*gate);
                   });
    return lst;
  }

  inline void save_data(qbit q) { _data.insert(std::make_unique<qbit>(q)); }

  inline void save_ancilla(qbit q) {
    _ancilla.insert(std::make_unique<qbit>(q));
    auto it = _ancilla.begin();
  }

  std::set<ast::ptr<qbit>>::iterator ancilla_begin() const {
    return _ancilla.begin();
  }
  std::set<ast::ptr<qbit>>::iterator ancilla_end() const {
    return _ancilla.end();
  }
};

// pauli_string function
inline ast::ptr<ast::Stmt>
pauli_string(std::initializer_list<pauli_literal> paulis) {
  tools_v1::parser::Position pos;
  std::vector<ast::VarAccess> qubits;
  std::vector<ast::PauliType> pauli_types;

  for (const auto &p : paulis) {
    qubits.emplace_back(pos, "q", p.qubit_index());
    if (p.type() == "X") {
      pauli_types.push_back(ast::PauliType::X);
    } else if (p.type() == "Y") {
      pauli_types.push_back(ast::PauliType::Y);
    } else if (p.type() == "Z") {
      pauli_types.push_back(ast::PauliType::Z);
    }
  }

  return ast::PauliString::create(pos, std::move(qubits),
                                  std::move(pauli_types));
}

inline circuit prepare(int k, std::string reg_name = "a") {
  circuit c;
  tools_v1::parser::Position pos;

  for (int i = 0; i < k; ++i) {
    auto q = ast::VarAccess(pos, "a", i);
    auto h =
        ast::DeclaredGate::create(pos, "h", std::vector<ast::ptr<ast::Expr>>(),
                                  std::vector<ast::VarAccess>{q});
    c.push_back(std::move(h));
  }

  return c;
}

inline circuit reverse_circuit(const circuit &c) {
  circuit result;
  for (auto it = c.rbegin(); it != c.rend(); ++it) {
    result.push_back(tools_v1::ast::object::clone(**it));
  }
  return result;
}

[[deprecated("This function requires fixing: does not implement dagger.")]]
inline circuit dagger_circuit(const circuit &c) {
  // FIX:
  circuit result;
  for (auto it = c.rbegin(); it != c.rend(); ++it) {
    result.push_back(tools_v1::ast::object::clone(**it));
  }
  return result;
}

// Convenience array indexing for qubits
struct qbit_indexer {
  qbit operator[](int index) const { return qbit(index); }
};

inline qbit_indexer qbit_access;

inline ast::ptr<ast::Gate> hadamard(const qbit &q) {
  tools_v1::parser::Position pos;
  auto qubit = q.to_va();
  return ast::DeclaredGate::create(pos, "h", std::vector<ast::ptr<ast::Expr>>(),
                                   std::vector<ast::VarAccess>{qubit});
}

inline ast::ptr<ast::Gate> rz(const qbit &q, ast::ptr<ast::Expr> alpha) {
  tools_v1::parser::Position pos;
  auto qubit = q.to_va();
  std::vector<ast::ptr<ast::Expr>> args;
  args.push_back(std::move(alpha));
  return ast::DeclaredGate::create(pos, "rz", std::move(args),
                                   std::vector<ast::VarAccess>{qubit});
}

inline ast::ptr<ast::Gate> cnot(const qbit &c, const qbit &t) {
  tools_v1::parser::Position pos;
  auto ctl = c.to_va();
  auto tgt = t.to_va();
  return ast::CNOTGate::create(pos, std::move(ctl), std::move(tgt));
}

// Output streaming operator for circuit
inline std::ostream &operator<<(std::ostream &os, const circuit &c) {
  tools_v1::parser::Position pos;
  std::list<ast::ptr<ast::Stmt>> body;

  auto qreg = ast::RegisterDecl::create(pos, "q", true, 16);
  body.push_back(std::move(qreg));

  auto areg = ast::RegisterDecl::create(pos, "a", true, 16);
  body.push_back(std::move(areg));

  for (const auto &gate : c) {
    if (gate) {
      GateToStmt cloner;
      gate->accept(cloner);
      if (cloner.cloned_gate) {
        body.push_back(std::move(cloner.cloned_gate));
      }
    }
  }

  auto program = ast::Program::create(pos, true, std::move(body), 0, 16);
  if (program) {
    os << *program;
  } else {
    os << "Empty circuit";
  }
  return os;
}

} // namespace tools_v1::tools

#endif
