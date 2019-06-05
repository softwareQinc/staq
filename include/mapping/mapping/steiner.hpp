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

#pragma once

#include "ast/replacer.hpp"
#include "synthesis/linear_reversible.hpp"
#include "synthesis/cnot_dihedral.hpp"
#include "mapping/device.hpp"
#include "utils/templates.hpp"

#include <vector>

namespace synthewareQ {
namespace mapping {

  /** 
   * \brief Steiner tree based resynthesizing mapper
   *
   * Resynthesizes an entire circuit by breaking into cnot-dihedral "chunks"
   * and resynthesizing those using gray-synth (arXiv:1712.01859) extended
   * with a device dependent mapping technique based on Steiner trees 
   * (arXiv:1904.01972)
   *
   * Assumes the circuit has already been laid out onto a single register 
   * with name given in the configuration
   */
  void steiner_mapping(Device&, ast::Program&);

  /* Implementation */
  class SteinerMapper final : public ast::Replacer {
  public:
    struct config {
      std::string register_name = "q";
    };

    SteinerMapper(Device& device) : Replacer(), device_(device) {
      permutation_ = synthesis::linear_op<bool>(device.qubits_, std::vector<bool>(device.qubits_, false));
      for (auto i = 0; i < device.qubits_; i++) {
        permutation_[i][i] = true;
      }
    }

    // Ignore declarations if they were left in during inlining
    void visit(ast::GateDecl&) override {}
    void visit(ast::OracleDecl&) override {}

    void visit(ast::Program& prog) override {
      Replacer::visit(prog);
      
      // Synthesize the last leg
      for (auto& gate : synthesis::gray_steiner(phases_, permutation_, device_)) {
        std::visit(utils::overloaded {
            [this, &prog](std::pair<int, int>& cx) {
              if (device_.coupled(cx.first, cx.second)) {
                prog.body().emplace_back(generate_cnot(cx.first, cx.second, prog.pos()));
              } else if (device_.coupled(cx.second, cx.first)) {
                prog.body().emplace_back(generate_hadamard(cx.first, prog.pos()));
                prog.body().emplace_back(generate_hadamard(cx.second, prog.pos()));
                prog.body().emplace_back(generate_cnot(cx.first, cx.second, prog.pos()));
                prog.body().emplace_back(generate_hadamard(cx.first, prog.pos()));
                prog.body().emplace_back(generate_hadamard(cx.second, prog.pos()));
              } else {
                throw std::logic_error("CNOT between non-coupled vertices!");
              }
            },
            [this, &prog](std::pair<utils::Angle, int>& rz) {
              prog.body().emplace_back(generate_rz(rz.first, rz.second, prog.pos()));
            }}, gate);
      }
    }

    std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::CNOTGate& gate) override {
      auto ctrl = get_index(gate.ctrl());
      auto tgt  = get_index(gate.tgt());

      if (in_bounds(ctrl) && in_bounds(tgt)) {
        synthesis::operator^=(permutation_[tgt], permutation_[ctrl]);
      } else {
        throw std::logic_error("CNOT argument(s) out of device bounds!");
      }

      // Delete the gate
      return std::list<ast::ptr<ast::Gate> >();
    }

    std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::UGate& gate) override {
      if (is_zero(gate.theta()) && is_zero(gate.phi())) {
        // It's a z-axis rotation
        auto angle = gate.lambda().constant_eval();
        if (!angle) {
          throw std::logic_error("Rotation angle is not constant!");
        }

        auto idx   = get_index(gate.arg());

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], *angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else {
        return flush<ast::Gate>(gate);
      }
    }
    std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::DeclaredGate& gate) override {
      auto name = gate.name();

      if (name == "rz" || name == "u1") {
        auto angle = gate.carg(0).constant_eval();
        if (!angle) {
          throw std::logic_error("Rotation angle is not constant!");
        }

        auto idx = get_index(gate.qarg(0));
        if (in_bounds(idx)) {
          add_phase(permutation_[idx], *angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else if (name == "z") {
        auto angle = utils::angles::pi;
        auto idx = get_index(gate.qarg(0));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else if (name == "s") {
        auto angle = utils::angles::pi_half;
        auto idx = get_index(gate.qarg(0));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else if (name == "sdg") {
        auto angle = -utils::angles::pi_half;
        auto idx = get_index(gate.qarg(0));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else if (name == "t") {
        auto angle = utils::angles::pi_quarter;
        auto idx = get_index(gate.qarg(0));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else if (name == "tdg") {
        auto angle = -utils::angles::pi_quarter;
        auto idx = get_index(gate.qarg(0));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return std::list<ast::ptr<ast::Gate> >();
      } else {
        return flush<ast::Gate>(gate);
      }
    }

    // Always generate a synthesis event
    std::optional<std::list<ast::ptr<ast::Stmt> > > replace(ast::IfStmt& stmt) override {
      return flush<ast::Stmt>(stmt);
    }
    std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::BarrierGate& stmt) override {
      return flush<ast::Gate>(stmt);
    }
    std::optional<std::list<ast::ptr<ast::Stmt> > > replace(ast::MeasureStmt& stmt) override {
      return flush<ast::Stmt>(stmt);
    }
    std::optional<std::list<ast::ptr<ast::Stmt> > > replace(ast::ResetStmt& stmt) override {
      return flush<ast::Stmt>(stmt);
    }

  private:
    Device device_;
    config config_;

    // Accumulating data
    std::list<synthesis::phase_term> phases_;
    synthesis::linear_op<bool> permutation_;

    // NOTE: this algorithm does NOT do any phase merging. That should be handled by the rotation
    // merging optimization
    void add_phase(std::vector<bool> parity, utils::Angle angle) {
      phases_.push_back(std::make_pair(parity, angle));
    }

    // Flushes a cnot-dihedral operator (i.e. phases + permutation) to the circuit
    // before the given node
    template <typename T>
    std::list<ast::ptr<T> > flush(T& node) {
      std::list<ast::ptr<T> > ret;
      
      // Synthesize circuit
      for (auto& gate : synthesis::gray_steiner(phases_, permutation_, device_)) {
        std::visit(utils::overloaded {
            [&ret, this, &node](std::pair<int, int>& cx) {
              if (device_.coupled(cx.first, cx.second)) {
                ret.emplace_back(generate_cnot(cx.first, cx.second, node.pos()));
              } else if (device_.coupled(cx.second, cx.first)) {
                ret.emplace_back(generate_hadamard(cx.first, node.pos()));
                ret.emplace_back(generate_hadamard(cx.second, node.pos()));
                ret.emplace_back(generate_cnot(cx.first, cx.second, node.pos()));
                ret.emplace_back(generate_hadamard(cx.first, node.pos()));
                ret.emplace_back(generate_hadamard(cx.second, node.pos()));
              } else {
                throw std::logic_error("CNOT between non-coupled vertices!");
              }
            },
            [&ret, this, &node](std::pair<utils::Angle, int>& rz) {
              ret.emplace_back(generate_rz(rz.first, rz.second, node.pos()));
            }}, gate);
      }
      ret.emplace_back(ast::ptr<T>(node.clone()));

      // Reset the cnot-dihedral circuit
      phases_.clear();
      for (auto i = 0; i < device_.qubits_; i++) {
        for (auto j = 0; j < device_.qubits_; j++) {
          permutation_[i][j] = i == j ? true : false;
        }
      }

      return ret;
    }

    bool in_bounds(int i) { return 0 <= i && i < device_.qubits_; }

    bool is_zero(ast::Expr& expr) {
      auto val = expr.constant_eval();
      return val && (*val == 0);
    }

    int get_index(ast::VarAccess& va) {
      if (va.offset())
        return *(va.offset());
      else
        throw std::logic_error("Gate argument is not a register dereference!");
    }

    // Gate generation
    ast::ptr<ast::CNOTGate> generate_cnot(int i, int j, parser::Position pos) {
      auto ctrl = ast::VarAccess(pos, config_.register_name, i);
      auto tgt = ast::VarAccess(pos, config_.register_name, j);
      return std::make_unique<ast::CNOTGate>(ast::CNOTGate(pos, std::move(ctrl), std::move(tgt)));
    }
      
    ast::ptr<ast::UGate> generate_hadamard(int i, parser::Position pos) {
      auto tgt = ast::VarAccess(pos, config_.register_name, i);

      auto tmp1  = std::make_unique<ast::PiExpr>(ast::PiExpr(pos));
      auto tmp2  = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 2));
      auto theta = std::make_unique<ast::BExpr>(ast::BExpr(
                     pos, std::move(tmp1), ast::BinaryOp::Divide, std::move(tmp2)));
      auto phi = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 0));
      auto lambda = std::make_unique<ast::PiExpr>(ast::PiExpr(pos));

      return std::make_unique<ast::UGate>(ast::UGate(
        pos, std::move(theta), std::move(phi), std::move(lambda), std::move(tgt)));
    }

    ast::ptr<ast::UGate> generate_rz(utils::Angle angle, int i, parser::Position pos) {
      auto tgt = ast::VarAccess(pos, config_.register_name, i);

      auto theta = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 0));
      auto phi = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 0));
      auto lambda = ast::angle_to_expr(angle);

      return std::make_unique<ast::UGate>(ast::UGate(
        pos, std::move(theta), std::move(phi), std::move(lambda), std::move(tgt)));
    }
  };
    
  void steiner_mapping(Device& device, ast::Program& prog) {
    SteinerMapper mapper(device);
    prog.accept(mapper);
  }
}
}
