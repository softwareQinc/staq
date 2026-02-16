#ifndef NODE_CONVERSION_HPP_
#define NODE_CONVERSION_HPP_

#include <tools_v1/ast/cloneable.hpp>
#include <tools_v1/ast/visitor.hpp>
#include <tools_v1/ast/stmt.hpp>
#include <tools_v1/ast/control_gate.hpp>

// Convert Stmt to Gate
struct GateConverter : public tools_v1::ast::Visitor {
  tools_v1::ast::ptr<tools_v1::ast::Gate> converted_gate;
  void visit(tools_v1::ast::DeclaredGate &gate) override {
    converted_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::PauliString &gate) override {
    converted_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::VarAccess &) override {}
  void visit(tools_v1::ast::BExpr &) override {}
  void visit(tools_v1::ast::UExpr &) override {}
  void visit(tools_v1::ast::PiExpr &) override {}
  void visit(tools_v1::ast::IntExpr &) override {}
  void visit(tools_v1::ast::RealExpr &) override {}
  void visit(tools_v1::ast::VarExpr &) override {}
  void visit(tools_v1::ast::MeasureStmt &) override {}
  void visit(tools_v1::ast::ResetStmt &) override {}
  void visit(tools_v1::ast::IfStmt &) override {}
  void visit(tools_v1::ast::UGate &) override {}
  void visit(tools_v1::ast::CNOTGate &) override {}
  void visit(tools_v1::ast::BarrierGate &) override {}
  void visit(tools_v1::ast::PhaseGate &) override {}
  void visit(tools_v1::ast::ExpPauli &) override {}
  void visit(tools_v1::ast::ControlGate &) override {}
  void visit(tools_v1::ast::MultiControlGate &gate) override {
    converted_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::GateDecl &) override {}
  void visit(tools_v1::ast::OracleDecl &) override {}
  void visit(tools_v1::ast::RegisterDecl &) override {}
  void visit(tools_v1::ast::AncillaDecl &) override {}
  void visit(tools_v1::ast::Program &) override {}
};

inline tools_v1::ast::ptr<tools_v1::ast::Gate> stmt_to_gate(tools_v1::ast::Stmt &st){
  GateConverter gc;
  st.accept(gc);
  return std::move(gc.converted_gate);
}

struct GateCloner : public tools_v1::ast::Visitor {
  tools_v1::ast::ptr<tools_v1::ast::Gate> cloned_gate;
  void visit(tools_v1::ast::DeclaredGate &gate) override {
    cloned_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::PauliString &gate) override {
    cloned_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::VarAccess &) override {}
  void visit(tools_v1::ast::BExpr &) override {}
  void visit(tools_v1::ast::UExpr &) override {}
  void visit(tools_v1::ast::PiExpr &) override {}
  void visit(tools_v1::ast::IntExpr &) override {}
  void visit(tools_v1::ast::RealExpr &) override {}
  void visit(tools_v1::ast::VarExpr &) override {}
  void visit(tools_v1::ast::MeasureStmt &) override {}
  void visit(tools_v1::ast::ResetStmt &) override {}
  void visit(tools_v1::ast::IfStmt &) override {}
  void visit(tools_v1::ast::UGate &) override {}
  void visit(tools_v1::ast::CNOTGate &gate) override {
    cloned_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::BarrierGate &) override {}
  void visit(tools_v1::ast::PhaseGate &) override {}
  void visit(tools_v1::ast::ExpPauli &) override {}
  void visit(tools_v1::ast::ControlGate &) override {}
  void visit(tools_v1::ast::MultiControlGate &) override {}
  void visit(tools_v1::ast::GateDecl &) override {}
  void visit(tools_v1::ast::OracleDecl &) override {}
  void visit(tools_v1::ast::RegisterDecl &) override {}
  void visit(tools_v1::ast::AncillaDecl &) override {}
  void visit(tools_v1::ast::Program &) override {}
};

struct GateToStmt : public tools_v1::ast::Visitor {
  tools_v1::ast::ptr<tools_v1::ast::Stmt> cloned_gate;

  void visit(tools_v1::ast::DeclaredGate &gate) override {
    std::vector<tools_v1::ast::ptr<tools_v1::ast::Expr>> c_args;
    for (int i = 0; i < gate.num_cargs(); ++i) {
      c_args.push_back(tools_v1::ast::object::clone(gate.carg(i)));
    }
    std::vector<tools_v1::ast::VarAccess> q_args;
    for (int i = 0; i < gate.num_qargs(); ++i) {
      q_args.push_back(gate.qarg(i));
    }
    cloned_gate = tools_v1::ast::DeclaredGate::create(
        gate.pos(), gate.name(), std::move(c_args), std::move(q_args));
  }

  void visit(tools_v1::ast::PauliString &gate) override {
    std::vector<tools_v1::ast::VarAccess> qubits;
    for (const auto &qubit : gate.qargs()) {
      qubits.push_back(qubit);
    }
    std::vector<tools_v1::ast::PauliType> paulis;
    for (int i = 0; i < gate.num_qargs(); ++i) {
      paulis.push_back(tools_v1::ast::PauliType::X);
    }
    cloned_gate = tools_v1::ast::PauliString::create(gate.pos(), std::move(qubits),
                                           std::move(paulis));
  }

  void visit(tools_v1::ast::MultiControlGate &gate) override {
    std::vector<tools_v1::ast::VarAccess> ctrl1;
    for (const auto &qubit : gate.ctrl1()) {
      ctrl1.push_back(qubit);
    }
    std::vector<tools_v1::ast::VarAccess> ctrl2;
    for (const auto &qubit : gate.ctrl2()) {
      ctrl2.push_back(qubit);
    }
    GateToStmt target_cloner;
    gate.target_gate().accept(target_cloner);
    tools_v1::ast::ptr<tools_v1::ast::Gate> target_gate(
        static_cast<tools_v1::ast::Gate *>(target_cloner.cloned_gate.release()));
    cloned_gate = tools_v1::ast::MultiControlGate::create(gate.pos(), std::move(ctrl1),
                                                std::move(ctrl2),
                                                std::move(target_gate));
  }

  void visit(tools_v1::ast::VarAccess &) override {}
  void visit(tools_v1::ast::BExpr &) override {}
  void visit(tools_v1::ast::UExpr &) override {}
  void visit(tools_v1::ast::PiExpr &) override {}
  void visit(tools_v1::ast::IntExpr &) override {}
  void visit(tools_v1::ast::RealExpr &) override {}
  void visit(tools_v1::ast::VarExpr &) override {}
  void visit(tools_v1::ast::MeasureStmt &) override {}
  void visit(tools_v1::ast::ResetStmt &) override {}
  void visit(tools_v1::ast::IfStmt &) override {}
  void visit(tools_v1::ast::UGate &) override {}
  void visit(tools_v1::ast::CNOTGate &gate) override {
    cloned_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::BarrierGate &) override {}
  void visit(tools_v1::ast::PhaseGate &) override {}
  void visit(tools_v1::ast::ExpPauli &) override {}
  void visit(tools_v1::ast::ControlGate &gate) override {
    cloned_gate = tools_v1::ast::object::clone(gate);
  }
  void visit(tools_v1::ast::GateDecl &) override {}
  void visit(tools_v1::ast::OracleDecl &) override {}
  void visit(tools_v1::ast::RegisterDecl &) override {}
  void visit(tools_v1::ast::AncillaDecl &) override {}
  void visit(tools_v1::ast::Program &) override {}
};

#endif
