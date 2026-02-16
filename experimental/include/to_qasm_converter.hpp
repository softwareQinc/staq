#ifndef TO_QASM_CONVERTER_HPP_
#define TO_QASM_CONVERTER_HPP_

#include <tools_v1/ast/control_gate.hpp>
#include <tools_v1/ast/expr.hpp>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/ast/visitor.hpp>
#include <algorithm>

namespace tools_v1 {
namespace ast {

class QASMify : public Visitor {
private:
  int cnt_mcg = 0;
  int max_a_tof = 0;
  ptr<Program> new_prog;

public:
  QASMify() {
    tools_v1::parser::Position pos;
    bool std_include = true;
    int num_cbits = 0;
    int num_qbits = 0;
    new_prog = Program::create(pos, std_include, {}, 0, 0);
  }
  // Variables
  void visit(VarAccess &) override {}
  // Expressions
  void visit(BExpr &) override {}
  void visit(UExpr &) override {}
  void visit(PiExpr &) override {}
  void visit(IntExpr &) override {}
  void visit(RealExpr &) override {}
  void visit(VarExpr &) override {}
  // Statements
  void visit(MeasureStmt &ms) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(ms));
  }
  void visit(ResetStmt &rs) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(rs));
  }
  void visit(IfStmt &is) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(is));
  }
  // Gates
  void visit(UGate &ug) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(ug));
  }
  void visit(CNOTGate &cg) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(cg));
  }
  void visit(BarrierGate &bg) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(bg));
  }
  void visit(DeclaredGate &dg) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(dg));
  }

  // New Gates
  void visit(PauliString &ps) override {
    auto f = [this](VarAccess &va, PauliType pt) {
      tools_v1::parser::Position pos;
      ptr<DeclaredGate> dg;
      switch (pt) {
      case PauliType::I:
        break;
      case PauliType::X:
        dg = DeclaredGate::create(pos, "x", {}, {std::move(va)});
      case PauliType::Y:
        dg = DeclaredGate::create(pos, "y", {}, {std::move(va)});
      case PauliType::Z:
        dg = DeclaredGate::create(pos, "z", {}, {std::move(va)});
      }
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*dg));
    };
    ps.foreach_pauli(f);
  }

  void visit(PhaseGate &phg) override {
    // This is a r_z rotation
    ptr<Expr> phg_angle = tools_v1::ast::object::clone(phg.angle());
    std::vector<VarAccess> phg_qargs = phg.qargs();

    tools_v1::parser::Position pos;
    std::vector<ptr<Expr>> phg_cargs;
    phg_cargs.emplace_back(std::move(phg_angle));

    ptr<DeclaredGate> dg = DeclaredGate::create(pos, "rz", std::move(phg_cargs),
                                                std::move(phg_qargs));

    this->new_prog->body().push_back(tools_v1::ast::object::clone(*dg));
  }

  void visit(ExpPauli &ep) override {
    ptr<Expr> ep_angle = tools_v1::ast::object::clone(ep.angle());
    std::vector<VarAccess> ep_qargs = ep.qargs();
    std::vector<PauliType> ep_paulis = ep.paulis();
    // helper: diagonalizes paulis translating them into z
    auto z_pauli_translator = [this](VarAccess va, PauliType pt,
                                     bool dag = false) -> void {
      tools_v1::parser::Position pos;
      assert(tools_v1::ast::PauliType::I != pt);
      ptr<DeclaredGate> dg_full;
      switch (pt) {
      case PauliType::I:
        throw "pauli conversion from I to Z failed";
        break;
      case PauliType::Z:
        dg_full =
            tools_v1::ast::DeclaredGate::create(pos, "I", {}, {std::move(va)});
      case PauliType::X:
        dg_full =
            tools_v1::ast::DeclaredGate::create(pos, "h", {}, {std::move(va)});
      case PauliType::Y:
        ptr<DeclaredGate> dg0;
        ptr<DeclaredGate> dg;
        // TODO: check the correct order
        dg0 = (!dag ? tools_v1::ast::DeclaredGate::create(pos, "h", {},
                                                          {std::move(va)})
                    : tools_v1::ast::DeclaredGate::create(pos, "sdag", {},
                                                          {std::move(va)}));
        dg = (!dag ? tools_v1::ast::DeclaredGate::create(pos, "s", {},
                                                         {std::move(va)})
                   : tools_v1::ast::DeclaredGate::create(pos, "h", {},
                                                         {std::move(va)}));
        std::vector<ptr<DeclaredGate>> dg_full;
        dg_full.emplace_back(tools_v1::ast::object::clone(*dg0));
        dg_full.emplace_back(tools_v1::ast::object::clone(*dg));
      };
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*dg_full));
    };
    // UGLY: copied function from MC gate
    auto create_cnot = [this](VarAccess c, VarAccess t) {
      tools_v1::parser::Position pos;
      auto cnot = CNOTGate::create(pos, std::move(c), std::move(t));
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*cnot));
    };

    tools_v1::parser::Position pos;
    ptr<Expr> double_angle = tools_v1::ast::BExpr::create(
        pos, tools_v1::ast::RealExpr::create(pos, 2.0),
        tools_v1::ast::BinaryOp::Times,
        tools_v1::ast::object::clone(*ep_angle));
    if (ep_paulis.size() >= 1) {
      // case of non-trivial pauli string
      assert(ep_qargs.size() == ep_paulis.size());
      int L = ep_paulis.size();
      // U operation
      z_pauli_translator(ep_qargs[0], ep_paulis[0], false);
      for (int i = 1; i < L; ++i) {
        z_pauli_translator(ep_qargs[i - 1], ep_paulis[i - 1], false);
        create_cnot(ep_qargs[i - 1], ep_qargs[i]);
      }
      std::vector<ptr<Expr>> vec_ptr_expr;
      vec_ptr_expr.push_back(std::move(double_angle));
      // rotation
      this->new_prog->body().push_back(tools_v1::ast::DeclaredGate::create(
          pos, "rz", std::move(vec_ptr_expr), {std::move(ep_qargs[L - 1])}));
      // undo U operation: U^dag
      for (int i = L - 1; i > 0; --i) {
        create_cnot(ep_qargs[i - 1], ep_qargs[i]);
        z_pauli_translator(ep_qargs[i], ep_paulis[i], true);
      }
      z_pauli_translator(ep_qargs[0], ep_paulis[0], true);
    }

    else { // case of empty pauli string
      throw "ExpPauli filed. No Pauli String found";
    }
  }

  void visit(ControlGate &cg) override {
    VarAccess ctrl = cg.ctrl();
    auto t_gate = tools_v1::ast::object::clone(cg.target_gate());

    tools_v1::parser::Position pos;
    ptr<MultiControlGate> mc_gate =
        MultiControlGate::create(pos, {std::move(ctrl)}, {}, std::move(t_gate));

    mc_gate->accept(*this);
  }

  void visit(MultiControlGate &gate) override { // <-
    tools_v1::parser::Position pos;

    auto create_hadamard = [this](VarAccess &q) {
      tools_v1::parser::Position pos;
      auto h = DeclaredGate::create(pos, "h", {}, {std::move(q)});
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*h));
    };

    auto create_cnot = [this](VarAccess &c, VarAccess &t) {
      tools_v1::parser::Position pos;
      auto cnot = CNOTGate::create(pos, std::move(c), std::move(t));
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*cnot));
    };

    auto create_t_tdag = [this](VarAccess &q, bool dag = false) {
      tools_v1::parser::Position pos;
      ptr<DeclaredGate> dg;
      if (!dag)
        dg = DeclaredGate::create(pos, "t", {}, {std::move(q)});
      else
        dg = DeclaredGate::create(pos, "tdg", {}, {std::move(q)});
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*dg));
    };

    auto create_toffoli = [this, &create_hadamard, &create_cnot,
                           &create_t_tdag](VarAccess &c0, VarAccess &c1,
                                           VarAccess &t) {
      tools_v1::parser::Position pos;
      create_hadamard(t);
      create_cnot(c1, t);
      create_t_tdag(t, true);
      create_cnot(c0, t);
      create_t_tdag(t, false);
      create_cnot(c1, t);
      create_t_tdag(t, true);
      create_cnot(c0, t);
      create_t_tdag(c1, true);
      create_t_tdag(t, false);
      create_cnot(c0, c1);
      create_t_tdag(c1, true);
      create_cnot(c0, c1);
      create_t_tdag(c0, false);

      // S = T^2
      create_t_tdag(c1, false);
      create_t_tdag(c1, false);

      create_hadamard(t);
    };

    auto create_pauli_to_X_U = [this](VarAccess &q, PauliType p,
                                      bool dag = false) -> void {
      tools_v1::parser::Position pos;
      assert(tools_v1::ast::PauliType::I != p);
      ptr<DeclaredGate> dg;
      switch (p) {
      case tools_v1::ast::PauliType::I:
        throw "pauli conversion from I to X failed";
        break;
      case tools_v1::ast::PauliType::X:
        dg = tools_v1::ast::DeclaredGate::create(pos, "I", {}, {std::move(q)});
      case tools_v1::ast::PauliType::Y:
        dg = (!dag ? tools_v1::ast::DeclaredGate::create(pos, "s", {},
                                                         {std::move(q)})
                   : tools_v1::ast::DeclaredGate::create(pos, "sdag", {},
                                                         {std::move(q)}));
      case tools_v1::ast::PauliType::Z:
        dg = tools_v1::ast::DeclaredGate::create(pos, "h", {}, {std::move(q)});
      };
      this->new_prog->body().push_back(tools_v1::ast::object::clone(*dg));
    };

    auto pack_controls = [this](MultiControlGate &gate,
                                MultiControlGate *&ptr_t_gate) {
      std::vector<VarAccess> ctrl1_outer = gate.ctrl1();
      std::vector<VarAccess> ctrl1_inner = ptr_t_gate->ctrl1();

      std::vector<VarAccess> ctrl1_new;
      ctrl1_new.reserve(ctrl1_outer.size() + ctrl1_inner.size());
      for (auto &va : ctrl1_outer) {
        ctrl1_new.push_back(va);
      }

      std::vector<VarAccess> ctrl2_outer = gate.ctrl2();
      std::vector<VarAccess> ctrl2_inner = ptr_t_gate->ctrl2();

      std::vector<VarAccess> ctrl2_new;
      ctrl2_new.reserve(ctrl2_outer.size() + ctrl2_inner.size());
      for (auto &va : ctrl2_outer) {
        ctrl2_new.push_back(va);
      }

      tools_v1::parser::Position pos;
      ptr<Gate> cloned_t_gate =
          tools_v1::ast::object::clone(ptr_t_gate->target_gate());

      return tools_v1::ast::MultiControlGate::create(pos, std::move(ctrl1_new),
                                                     std::move(ctrl2_new),
                                                     std::move(cloned_t_gate));
    };

    auto ptr_t_gate = dynamic_cast<MultiControlGate *>(&gate.target_gate());
    if (ptr_t_gate != nullptr) {
      ptr<MultiControlGate> new_gate = pack_controls(gate, ptr_t_gate);
      new_gate->accept(*this);
      return;
    }

    auto ps_ref = dynamic_cast<PauliString *>(&gate.target_gate());
    if (ps_ref != nullptr) {

      std::vector<VarAccess> new_ctrl1_ref = gate.ctrl1();
      std::vector<VarAccess> new_ctrl2_ref = gate.ctrl2();

      auto gen_toff_transpiler = [&new_ctrl1_ref, &new_ctrl2_ref,
                                  &create_hadamard, &create_cnot,
                                  &create_toffoli, &create_pauli_to_X_U,
                                  this](VarAccess &t, PauliType p) {
        // add unitary, U, so that P = UXU^dag
        create_pauli_to_X_U(t, p, false);

        // convert 0-control to 1-control
        for (auto &va : new_ctrl2_ref) {
          tools_v1::parser::Position pos;
          auto new_targ = va;
          auto dg = DeclaredGate::create(pos, "x", {}, {std::move(new_targ)});
          this->new_prog->body().push_back(std::move(dg));
        }

        // unify all ctrls (prev 0 and prev 1) into one vector of ctrls for
        // generalized toffoli
        std::vector<VarAccess> merge_ctrl;
        merge_ctrl.reserve(new_ctrl1_ref.size() + new_ctrl2_ref.size());
        for (auto &va : new_ctrl1_ref) {
          merge_ctrl.push_back(va);
        }
        for (auto &va : new_ctrl2_ref) {
          merge_ctrl.push_back(va);
        }

        // assert(merge_ctrl.size() >= 3);
        if (merge_ctrl.size() >= 3) {
          std::vector<VarAccess> a;
          a.reserve(merge_ctrl.size() - 1);
          max_a_tof = std::max(static_cast<int>(merge_ctrl.size()), max_a_tof);
          for (int i = 0; i < merge_ctrl.size() - 1; ++i) {
            tools_v1::parser::Position pos;
            a.emplace_back(pos, "a_tof", i);
          }

          create_toffoli(merge_ctrl[0], merge_ctrl[1], a[0]);
          for (int i = 2; i < merge_ctrl.size(); ++i) {
            create_toffoli(merge_ctrl[i], a[i - 2], a[i - 1]);
          }
          create_cnot(a.back(), t);
          for (int i = merge_ctrl.size() - 1; i >= 2; --i) {
            create_toffoli(merge_ctrl[i], a[i - 2], a[i - 1]);
          }
          create_toffoli(merge_ctrl[0], merge_ctrl[1], a[0]);
        } else if (merge_ctrl.size() == 2) {
          // add one toffoli
          create_toffoli(merge_ctrl[0], merge_ctrl[1], t);
        } else if (merge_ctrl.size() == 1) {
          // add cnot
          create_cnot(merge_ctrl[0], t);
        } else {
          tools_v1::parser::Position pos;
          auto new_targ = t;
          auto dg = tools_v1::ast::DeclaredGate::create(pos, "x", {},
                                                        {std::move(new_targ)});
          this->new_prog->body().push_back(std::move(dg));
        }

        // convert 0-controls to 1-controls, see above
        for (auto &va : new_ctrl2_ref) {
          tools_v1::parser::Position pos;
          auto new_targ = va;
          auto dg = DeclaredGate::create(pos, "x", {}, {std::move(new_targ)});
          this->new_prog->body().push_back(std::move(dg));
        }

        // add U^dag, so that P = UXU^dag, see above
        create_pauli_to_X_U(t, p, true);
      };
      ps_ref->foreach_pauli(gen_toff_transpiler);
      return;
    }

    auto dg_ref = dynamic_cast<DeclaredGate *>(&gate.target_gate());
    if (dg_ref != nullptr) {
      symbol cur_name = dg_ref->name();
      if (cur_name == "x") {
        auto c1 = gate.ctrl1();
        auto c2 = gate.ctrl2();
        auto qargs = dg_ref->qargs();
        std::vector<PauliType> paulis = {tools_v1::ast::PauliType::X};
        ptr<Gate> new_target_gate =
            PauliString::create(pos, std::move(qargs), std::move(paulis));
        auto new_multi_control_gate = MultiControlGate::create(
            pos, std::move(c1), std::move(c2), std::move(new_target_gate));
        new_multi_control_gate->accept(*this);
      }
      return;
    }

    throw "Not supported MultiControlGate transpilation operation.";
  }
  // Declarations
  void visit(GateDecl &) override {}
  void visit(OracleDecl &) override {}
  void visit(RegisterDecl & rg) override {
    new_prog->body().push_back(tools_v1::ast::object::clone(rg));
  }
  void visit(AncillaDecl &) override {}
  // Program
  void visit(Program &prog) override {
    auto it = prog.body().begin();
    while (it != prog.end()) {
      (*it)->accept(*this);
      ++it;
    }
    auto rg_tof = RegisterDecl::create(tools_v1::parser::Position(), "a_tof", true, max_a_tof);
    new_prog->body().push_front(std::move(rg_tof));
  }

  ptr<Program>& prog(){
    return new_prog;
  }

  void print_num_multicontrolgates() { std::cout << cnt_mcg << std::endl; }

  std::ostream &pretty_print(std::ostream &os) const {
    new_prog->pretty_print(os);
    return os;
  }
};

} // namespace ast
} // namespace tools_v1

#endif
