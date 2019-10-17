/*
 * This file is part of synthewareQ.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * \file optimization/rotation_folding.hpp
 * \brief Rotation folding algorithm
 */
#pragma once

#include "ast/visitor.hpp"
#include "ast/replacer.hpp"
#include "gates/channel.hpp"

#include <list>
#include <unordered_map>
#include <sstream>

namespace synthewareQ {
namespace optimization {

  /** 
   * \brief Rotation gate merging algorithm based on arXiv:1903.12456 
   *
   *  Returns a replacement list giving the nodes to the be replaced (or erased)
   */

  // TODO: Add option for global phase correction
  class RotationOptimizer final : public ast::Visitor {
    using Gatelib = gates::ChannelRepr<ast::VarAccess>;

  public:
    struct config {
      bool correct_global_phase = true;
    };

    RotationOptimizer() = default;
    RotationOptimizer(const config& params) : Visitor(), config_(params) {}
    ~RotationOptimizer() = default;

    std::unordered_map<int, std::list<ast::ptr<ast::Gate> > >
    run(ast::ASTNode& node) {
      reset();
      node.accept(*this);
      return std::move(replacement_list_);
    }

    /* Variables */
    void visit(ast::VarAccess&) {}

    /* Expressions */
    void visit(ast::BExpr&) {}
    void visit(ast::UExpr&) {}
    void visit(ast::PiExpr&) {}
    void visit(ast::IntExpr&) {}
    void visit(ast::RealExpr&) {}
    void visit(ast::VarExpr&) {}

    /* Statements */
    void visit(ast::MeasureStmt& stmt) {
      push_uninterp(Gatelib::Uninterp({ stmt.q_arg() }));
    }
    void visit(ast::ResetStmt& stmt) {
      push_uninterp(Gatelib::Uninterp({ stmt.arg() }));
    }
    void visit(ast::IfStmt& stmt) {
      mergeable_ = false;
      stmt.then().accept(*this);
      mergeable_ = true;
    }

    /* Gates */
    void visit(ast::UGate& gate) {
      push_uninterp(Gatelib::Uninterp({ gate.arg() }));
    }
    void visit(ast::CNOTGate& gate) {
      auto ctrl = gate.ctrl();
      auto tgt = gate.tgt();
      if (mergeable_) {
        current_clifford_ *= Gatelib::Clifford::cnot(ctrl, tgt);
      } else {
        push_uninterp(Gatelib::Uninterp({ ctrl, tgt }));
      }
    }
    void visit(ast::BarrierGate& gate) {
      push_uninterp(Gatelib::Uninterp({ gate.args() }));
    }
    void visit(ast::DeclaredGate& gate) {
      auto name = gate.name();

      if (mergeable_) {
        if (name == "cx")
          current_clifford_ *= Gatelib::Clifford::cnot(gate.qarg(0), gate.qarg(1));
        else if (name == "h")
          current_clifford_ *= Gatelib::Clifford::h(gate.qarg(0));
        else if (name == "x")
          current_clifford_ *= Gatelib::Clifford::x(gate.qarg(0));
        else if (name == "y")
          current_clifford_ *= Gatelib::Clifford::y(gate.qarg(0));
        else if (name == "z")
          current_clifford_ *= Gatelib::Clifford::z(gate.qarg(0));
        else if (name == "s")
          current_clifford_ *= Gatelib::Clifford::sdg(gate.qarg(0));
        else if (name == "sdg")
          current_clifford_ *= Gatelib::Clifford::s(gate.qarg(0));
        else if (name == "t") {
          auto rot = Gatelib::Rotation::t(gate.qarg(0));
          rotation_info info{ gate.uid(), rotation_info::axis::z, gate.qarg(0) };
          accum_.push_back(std::make_pair(info, rot.commute_left(current_clifford_)));
        } else if (name == "tdg") {
          auto rot = Gatelib::Rotation::tdg(gate.qarg(0));
          rotation_info info{ gate.uid(), rotation_info::axis::z, gate.qarg(0) };
          accum_.push_back(std::make_pair(info, rot.commute_left(current_clifford_)));
        } else if (name == "rz") {
          auto angle = gate.carg(0).constant_eval();

          if (angle) {
            auto rot = Gatelib::Rotation::rz(utils::Angle(*angle), gate.qarg(0));
            rotation_info info{ gate.uid(), rotation_info::axis::z, gate.qarg(0) };
            accum_.push_back(std::make_pair(info, rot.commute_left(current_clifford_)));
          } else {
            push_uninterp(Gatelib::Uninterp(gate.qargs()));
          }
        } else if (name == "rx") {
          auto angle = gate.carg(0).constant_eval();

          if (angle) {
            auto rot = Gatelib::Rotation::rx(utils::Angle(*angle), gate.qarg(0));
            rotation_info info{ gate.uid(), rotation_info::axis::x, gate.qarg(0) };
            accum_.push_back(std::make_pair(info, rot.commute_left(current_clifford_)));
          } else {
            push_uninterp(Gatelib::Uninterp(gate.qargs()));
          }
        } else if (name == "ry") {
          auto angle = gate.carg(0).constant_eval();

          if (angle) {
            auto rot = Gatelib::Rotation::ry(utils::Angle(*angle), gate.qarg(0));
            rotation_info info{ gate.uid(), rotation_info::axis::y, gate.qarg(0) };
            accum_.push_back(std::make_pair(info, rot.commute_left(current_clifford_)));
          } else {
            push_uninterp(Gatelib::Uninterp(gate.qargs()));
          }
        } else push_uninterp(Gatelib::Uninterp(gate.qargs()));
      } else {
        push_uninterp(Gatelib::Uninterp(gate.qargs()));
      }
    }

    /* Declarations */
    void visit(ast::GateDecl& decl) {
      // Initialize a new local state
      circuit_callback local_state;
      Gatelib::Clifford local_clifford;
      std::swap(accum_, local_state);
      std::swap(current_clifford_, local_clifford);

      // Process gate body
      decl.foreach_stmt([this](auto& stmt){ stmt.accept(*this); });
      accum_.push_back(current_clifford_);

      // Fold the gate body
      fold(accum_, true);

      // Reset the state
      std::swap(accum_, local_state);
      std::swap(current_clifford_, local_clifford);
    }
    void visit(ast::OracleDecl&) {}
    void visit(ast::RegisterDecl&) {}
    void visit(ast::AncillaDecl&) {}

    /* Program */
    void visit(ast::Program& prog) {
      prog.foreach_stmt([this](auto& stmt){ stmt.accept(*this); });
      accum_.push_back(current_clifford_);

      fold(accum_, config_.correct_global_phase);
    }

  private:
    // Store information necessary for generating a replacement of <node> with a
    // different angle
    struct rotation_info {
      enum class axis {x, y, z};
      
      int uid;
      axis rotation_axis;
      ast::VarAccess arg;
    };

    using circuit_callback = std::list<std::variant<Gatelib::Uninterp,
                                                    Gatelib::Clifford,
                                                    std::pair<rotation_info, Gatelib::Rotation> > >;

    config config_;
    std::unordered_map<int, std::list<ast::ptr<ast::Gate> > > replacement_list_;

    /* Algorithm state */
    circuit_callback accum_;             // The currently accumulating circuit (in channel repr.)
    bool mergeable_ = true;              // Whether we're in a context where a gate can be merged
    Gatelib::Clifford current_clifford_; // The current clifford operator
    // Note: current clifford is stored as the dagger of the actual Clifford gate
    // this is so that left commutation (i.e. conjugation) actually right-commutes
    // the rotation gate, allowing us to walk the circuit forwards rather than backwards
    // That is, we want to end up with the form
    //   C_0R_1R_2R_3...R_n
    // rather than the form
    //   R_n...R_3R_2R_1C_0
    // as in the paper

    void reset() {
      replacement_list_.clear();
      accum_.clear();
      mergeable_ = true;
      current_clifford_ = Gatelib::Clifford();
    }

    /* Phase two of the algorithm */
    void fold(circuit_callback& circuit, bool phase_correction) {
      auto global_phase = utils::angles::zero;
      ast::VarAccess* tgt = nullptr;
      std::list<ast::ptr<ast::Gate> >* subst_ref = nullptr;

      for (auto it = circuit.rbegin(); it != circuit.rend(); it++) {
        auto& op = *it;
        if (auto tmp = std::get_if<std::pair<rotation_info, Gatelib::Rotation> >(&op)) {
          auto [new_phase, new_R] = fold_forward(circuit, std::next(it), tmp->second);

          global_phase += new_phase;
          if (!(new_R == tmp->second)) {
            std::list<ast::ptr<ast::Gate> > subst;

            auto rot = alloc_rot(tmp->first, new_R.rotation_angle());
            if (rot)
              subst.emplace_back(rot);
            replacement_list_[tmp->first.uid] = std::move(subst);

            // WARNING: this is a massive hack so that the global phase correction can
            // be performed by the replacement engine. We append the final phase correction to
            // the last gate substitution in-place in the replacement list. Since we need
            // a qubit to apply the phase correction on, we select the qubit on which the
            // rotation itself was applied.
            tgt = &(tmp->first.arg);
            subst_ref = &(replacement_list_[tmp->first.uid]);
          }
        }
      }

      if (phase_correction && (global_phase != utils::angles::zero)) {
        if (global_phase == utils::angles::pi) {
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "z", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "z", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
        } else if (global_phase == utils::angles::pi_half) {
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "s", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "s", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
        } else if (global_phase == -utils::angles::pi_half) {
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "sdg", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "sdg", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
        } else if (global_phase == utils::angles::pi_quarter) {
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "h", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "s", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "h", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "s", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "h", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "s", {}, { *tgt }));
        } else if (global_phase == -utils::angles::pi_quarter) {
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "sdg", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "h", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "sdg", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "h", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "sdg", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "h", {}, { *tgt }));
        } else {
          std::vector<ast::ptr<ast::Expr> > tmp1;
          std::vector<ast::ptr<ast::Expr> > tmp2;
          tmp1.emplace_back(ast::angle_to_expr(global_phase));
          tmp2.emplace_back(ast::angle_to_expr(global_phase));

          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "rz", std::move(tmp1), { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "rz", std::move(tmp2), { *tgt }));
          subst_ref->emplace_back(new ast::DeclaredGate(parser::Position(), "x", {}, { *tgt }));
        }
      }
    }

    std::pair<utils::Angle, Gatelib::Rotation>
    fold_forward(circuit_callback& circuit, circuit_callback::reverse_iterator it, Gatelib::Rotation R)
    {
      // Tries to commute op backward as much as possible, merging with applicable
      // gates and deleting them as it goes
      // Note: We go backwards so that we only commute **left** past C^*/**right** past C

      auto phase = utils::angles::zero;
      bool cont = true;

      for (; cont && it != circuit.rend(); it++) {
        auto visitor = utils::overloaded {
          [this, it, &R, &phase, &circuit](std::pair<rotation_info, Gatelib::Rotation>& P) {
              auto res = R.try_merge(P.second);
              if (res) {
                auto &[new_phase, new_R] = res.value();
                phase += new_phase;
                R = new_R;

                // Delete R in circuit & the node
                replacement_list_[P.first.uid] = std::move(std::list<ast::ptr<ast::Gate> >()); 
                circuit.erase(std::next(it).base());

                return false;
              } else if (R.commutes_with(P.second)) {
                return true;
              } else {
                return false;
              }
            },
          [&R](Gatelib::Clifford& C) {
              R = R.commute_left(C); return true;
            },
          [&R](Gatelib::Uninterp& U) {
              if (!R.commutes_with(U)) return false;
              else return true;
            }
        };

        cont = std::visit(visitor, *it);
      }

      return std::make_pair(phase, R);
    }

    /* Utilities */
    void push_uninterp(Gatelib::Uninterp op) {
      accum_.push_back(current_clifford_);
      accum_.push_back(op);
      // Clear the current clifford
      current_clifford_ = Gatelib::Clifford();
    }

    // Assumes basic gates (x, y, z, s, sdg, t, tdg, rx, ry, rz) are defined
    ast::Gate* alloc_rot(const rotation_info& rinfo, const utils::Angle& theta) {
      if (theta.numeric_value() == 0)
        return nullptr;
      
      parser::Position pos;

      std::string name;
      std::vector<ast::ptr<ast::Expr> > cargs;
      std::vector<ast::VarAccess> qargs{ rinfo.arg };
      
      // Determine the name & classical arguments
      if (theta.is_numeric()) {
        // Angle is real-valued
        switch (rinfo.rotation_axis) {
        case rotation_info::axis::x:
          name = "rx";
          break;
        case rotation_info::axis::y:
          name = "ry";
          break;
        case rotation_info::axis::z:
          name = "rz";
          break;
        }

        cargs.emplace_back(ast::angle_to_expr(theta));
      } else {
        // Angle is of the form pi*(a/b) for a & b integers
        auto [a, b] = *(theta.symbolic_value());

        switch (rinfo.rotation_axis) {
        case rotation_info::axis::x:
          if ((a == 1) && (b == 1)) {
            // X gate
            name = "x";
          } else {
            // Rx gate
            name = "rx";
            cargs.emplace_back(ast::angle_to_expr(theta));
          }
          break;
        case rotation_info::axis::y:
          if ((a == 1) && (b == 1)) {
            // Y gate
            name = "y";
          } else {
            // Ry gate
            name = "ry";
            cargs.emplace_back(ast::angle_to_expr(theta));
          }
          break;
        case rotation_info::axis::z:
          if ((a == 1) && (b == 1)) {
            // Z gate
            name = "z";
          } else if (((a == 1) || (a == -3)) && (b == 2)) {
            // S gate
            name = "s";
          } else if (((a == -1) || (a == 3)) && (b == 2)) {
            // Sdg gate
            name = "sdg";
          } else if (((a == 1) || (a == -7)) && (b == 4)) {
            // T gate
            name = "t";
          } else if (((a == -1) || (a == 7)) && (b == 4)) {
            // Tdg gate
            name = "tdg";
          } else {
            // Rz gate
            name = "rz";
            cargs.emplace_back(ast::angle_to_expr(theta));
          }
          break;
        }
      }

      return new ast::DeclaredGate(pos, name, std::move(cargs), std::move(qargs));
    }
  };

  inline void fold_rotations(ast::ASTNode& node) {
    RotationOptimizer optimizer;

    auto res = optimizer.run(node);
    replace_gates(node, std::move(res));
  }

}
}
