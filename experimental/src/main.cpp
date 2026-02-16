#include <iostream>
#include <qasmtools/ast/decl.hpp>
#include <qasmtools/ast/expr.hpp>
#include <qasmtools/ast/program.hpp>
#include <qasmtools/ast/stmt.hpp>
#include <qasmtools/ast/visitor.hpp>
#include <qasmtools/parser/parser.hpp>
#include <vector>

qasmtools::ast::ptr<qasmtools::ast::Program> create_sample() {
  using namespace qasmtools::ast;

  qasmtools::parser::Position pos;
  std::list<ptr<Stmt>> body;
  auto qreg = RegisterDecl::create(pos, "q", true, 16);

  auto q0 = VarAccess(pos, "q", 0);
  auto q1 = VarAccess(pos, "q", 1);
  auto dg = DeclaredGate::create(pos, "h", {}, {std::move(q0)});
  body.push_back(std::move(dg));

  q0 = VarAccess(pos, "q", 0);
  q1 = VarAccess(pos, "q", 1);
  auto cx = CNOTGate::create(pos, std::move(q0), std::move(q1));

  body.push_back(std::move(cx));

  return Program::create(pos, true, std::move(body), 0, 16);
}

std::list<qasmtools::ast::ptr<qasmtools::ast::Stmt>>
multicontrolgate(std::vector<qasmtools::ast::VarAccess> control0,
                 std::vector<qasmtools::ast::VarAccess> control1,
                 std::vector<qasmtools::ast::Stmt> gates) {
  using namespace qasmtools::ast;

  std::list<ptr<Stmt>> body;

  // defn goes here

  return body;
}

std::list<qasmtools::ast::ptr<qasmtools::ast::Stmt>>
grover_rudolph(std::vector<double> vals,
               std::vector<qasmtools::ast::VarAccess> qubits) {
  using namespace qasmtools::ast;
  std::list<ptr<Stmt>> body;
  // implementation goes here
  return body;
}

std::list<qasmtools::ast::ptr<qasmtools::ast::Stmt>>
uniform_linear_combination(std::vector<qasmtools::ast::VarAccess> qubits) {
  using namespace qasmtools::ast;
  qasmtools::parser::Position pos;
  std::list<ptr<Stmt>> body;

  for (auto q : qubits) {
    auto h = DeclaredGate::create(pos, "h", {}, {std::move(q)});
    body.push_back(std::move(h));
  }

  return body;
}

// NOTES:
// 1.
//   uniform_linear_combination implements the creation of a state
//   ket(0^m) -> (ket(0...01) + ket(0..10) + ... + ket(1..11) ) / 2^(m/2)
//   this is part of the prepare subroutine in the linear combination of
//   unitaries. This will be suitable only for a uniformly weighted sum
//   TODO: OK for proof of concept; should be generalized later.

void run1() {
  // example 1
  auto program = create_sample();
  qasmtools::parser::Position pos;
  auto qubits = std::vector<qasmtools::ast::VarAccess>();
  qubits.reserve(16);
  for (int i = 0; i < 16; ++i)
    qubits.emplace_back(pos, "q", i);
  auto new_gates = uniform_linear_combination(qubits);
  program->body().splice(program->end(), new_gates);
  std::cout << *program << std::endl;
}

// void run2() {
//   using namespace qasmtools;
//   using namespace ast;
//   auto p = qasmtools::parser::Position();
//   auto q = VarAccess(p, "q", 0);
//   auto g = CNOTGate(p, VarAccess(p, "q", 1), VarAccess(p, "q", 2));
//   SimpleControl simple_control_gate(q, g);
// }

int main() {
  run1();
  return 0;
}
