#ifndef CIRCUIT_DAGGER_HPP_
#define CIRCUIT_DAGGER_HPP_

#include <node_conversion.hpp>
#include <string>
#include <tools_v1/ast/expr.hpp>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/ast/visitor.hpp>
#include <tools_v1/tools/staq_builder.hpp>
#include <vector>

namespace tools_v1 {
namespace ast {

std::list<ptr<Stmt>> gate_dagger(Gate &);
std::list<ptr<Stmt>> circuit_dagger(const tools::circuit &);

class CircuitDagger : public Visitor {
  ptr<Program> new_prog;
  ptr<Expr> neg(Expr &expr) {
    return UExpr::create(parser::Position(), UnaryOp::Neg, object::clone(expr));
  };

public:
  CircuitDagger() {
    parser::Position pos;
    bool std_include = false;
    std::list<ptr<Stmt>> body;
    new_prog = Program::create(pos, std_include, std::move(body), 0, 0);
  }
  std::list<ptr<Stmt>>& body(){
    return new_prog->body();
  }
  void visit(VarAccess &) override {
    throw "VarAccess is not supported when performing CircuitDagger operation.";
  }
  void visit(BExpr &) override {
    throw "BExpr is not supported when performing CircuitDagger operation.";
  }
  void visit(UExpr &) override {
    throw "UExpr is not supported when performing CircuitDagger operation.";
  }
  void visit(PiExpr &) override {
    throw "PiExpr is not supported when performing CircuitDagger operation.";
  }
  void visit(IntExpr &) override {
    throw "IntExpr is not supported when performing CircuitDagger operation.";
  }
  void visit(RealExpr &) override {
    throw "RealExpr is not supported when performing CircuitDagger operation.";
  }
  void visit(VarExpr &) override {
    throw "VarExpr is not supported when performing CircuitDagger operation.";
  }
  void visit(MeasureStmt &) override {
    throw "MeasureStmt is not supported when performing CircuitDagger "
          "operation.";
  }
  void visit(ResetStmt &) override {
    throw "ResetStmt is not supported when performing CircuitDagger operation.";
  }
  void visit(IfStmt &) override {
    throw "IfStmt is not supported when performing CircuitDagger operation.";
  }
  void visit(UGate &ug) override {
    parser::Position pos;
    auto new_ug =
        UGate::create(pos, neg(ug.lambda()), neg(ug.phi()), neg(ug.theta()),
                      std::move(*object::clone(ug.arg())));
    new_prog->body().push_front(std::move(new_ug));
  }
  void visit(CNOTGate &g) override {
    new_prog->body().push_front(object::clone(g));
  }
  void visit(BarrierGate &) override {
    throw "BarrierGate not supported when computing Circuit Dagger.";
  }
  void visit(DeclaredGate &dg) override {
    parser::Position pos;
    ptr<DeclaredGate> ng;
    std::vector<VarAccess> qargs;
    std::vector<std::string> hermitian = {"id", "cx", "x", "y", "z", "h"};
    for (auto &x : dg.qargs())
      qargs.emplace_back(std::move(*object::clone(x)));
    if (std::find(hermitian.begin(), hermitian.end(), dg.name()) !=
        hermitian.end()) {
      new_prog->body().push_front(object::clone(dg));
    } else if (dg.name() == "ry" || dg.name() == "rz" || dg.name() == "rx") {
      auto angle = neg(dg.carg(0));
      std::vector<ptr<Expr>> v;
      v.emplace_back(std::move(angle));
      ng = DeclaredGate::create(pos, dg.name(), std::move(v), std::move(qargs));
      new_prog->body().push_front(object::clone(*ng));
    } else if (dg.name() == "s") {
      ng = DeclaredGate::create(pos, "sdg", {}, std::move(qargs));
      new_prog->body().push_front(object::clone(*ng));
    } else if (dg.name() == "sdg") {
      ng = DeclaredGate::create(pos, "s", {}, std::move(qargs));
      new_prog->body().push_front(object::clone(*ng));
    } else if (dg.name() == "t") {
      ng = DeclaredGate::create(pos, "tdg", {}, std::move(qargs));
      new_prog->body().push_front(object::clone(*ng));
    } else if (dg.name() == "tdg") {
      ng = DeclaredGate::create(pos, "t", {}, std::move(qargs));
      new_prog->body().push_front(object::clone(*ng));
    } else {
      throw "DeclaredGate (" + dg.name() +
          ") not supported when computing Circuit Dagger.";
    }
  }
  void visit(PauliString &g) override {
    new_prog->body().push_front(object::clone(g));
  }
  void visit(PhaseGate &g) override {
    parser::Position pos;
    std::vector<VarAccess> qargs;
    for (auto &x : g.qargs())
      qargs.emplace_back(*object::clone(x));
    auto ng = PhaseGate::create(pos, neg(g.angle()), std::move(qargs));
    new_prog->body().push_front(std::move(ng));
  }
  void visit(ExpPauli &g) override {
    parser::Position pos;
    std::vector<VarAccess> qargs;
    std::vector<PauliType> paulis;
    for (auto &x : g.qargs())
      qargs.emplace_back(*object::clone(x));
    for (auto &x : g.paulis())
      paulis.push_back(x);

    auto ng = ExpPauli::create(pos, neg(g.angle()), std::move(qargs),
                               std::move(paulis));
    new_prog->body().push_front(std::move(ng));
  }
  void visit(ControlGate &cg) override {
    parser::Position pos;

    auto ntg = gate_dagger(cg.target_gate());
    for (auto &x : ntg) {
      ptr<Gate> g = stmt_to_gate(*x);
      VarAccess ctrl = *object::clone(cg.ctrl());
      auto ng = ControlGate::create(pos, std::move(ctrl), std::move(g));
      new_prog->body().push_front(std::move(ng));
    }
  }
  void visit(MultiControlGate &mcg) override {
    parser::Position pos;
    std::vector<VarAccess> ctrl1, ctrl2;
    for (auto &x : mcg.ctrl1()) {
      ctrl1.emplace_back(*object::clone(x));
    }
    for (auto &x : mcg.ctrl2()) {
      ctrl2.emplace_back(*object::clone(x));
    }
    auto ntg = gate_dagger(mcg.target_gate());
    for (auto &x : ntg) {
      ptr<Gate> g = stmt_to_gate(*x);
      auto ng = MultiControlGate::create(pos, std::move(ctrl1),
                                         std::move(ctrl2), std::move(g));
      new_prog->body().push_front(std::move(ng));
    }
  }
  //
  void visit(GateDecl &) override {
    throw "GateDecl is not supported when performing CircuitDagger operation.";
  }
  void visit(OracleDecl &) override {
    throw "OracleDecl is not supported when performing CircuitDagger "
          "operation.";
  }
  void visit(RegisterDecl &) override {
    throw "RegisterDecl is not supported when performing CircuitDagger "
          "operation.";
  }
  void visit(AncillaDecl &) override {
    throw "AncillaDecl is not supported when performing CircuitDagger "
          "operation.";
  }
  void visit(Program &prog) override {
    for (auto it = prog.begin(); it != prog.end(); ++it) {
      (*it)->accept(*this);
    }
  }
};

inline std::list<ptr<Stmt>> gate_dagger(Gate &g) {
  // temporary prog
  parser::Position pos;
  bool std_include = false;
  std::list<ptr<Stmt>> body;
  body.emplace_back(object::clone(g));
  ptr<Program> tmp_prog =
      Program::create(pos, std_include, std::move(body), 0, 0);
  // prepare CircuitDagger object
  CircuitDagger cd;
  tmp_prog->accept(cd);
  // return the body of the program (to be appended into )
  return std::move(cd.body());
}

inline std::list<ptr<Stmt>> circuit_dagger(const tools::circuit &c) {
  // temporary prog
  parser::Position pos;
  bool std_include = false;
  std::list<ptr<Stmt>> body;
  // body.emplace_back(object::clone(g));
  for (auto &x : c.body_list())
    body.push_back(std::move(x));
  ptr<Program> tmp_prog =
      Program::create(pos, std_include, std::move(body), 0, 0);
  // prepare CircuitDagger object
  CircuitDagger cd;
  tmp_prog->accept(cd);
  // return the body of the program (to be appended into )
  return std::move(cd.body());
}

} // namespace ast
} // namespace tools_v1

#endif
