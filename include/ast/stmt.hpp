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
 * \file ast/stmt.hpp
 * \brief openQASM statements
 */
#pragma once

#include "base.hpp"
#include "var.hpp"
#include "expr.hpp"

#include <functional>
#include <vector>

namespace synthewareQ {
namespace ast {

  /**
   * \class synthewareQ::ast::Stmt
   * \brief Base class for openQASM statements
   */
  class Stmt : public ASTNode {
  public:
    Stmt(parser::Position pos) : ASTNode(pos) {}
    virtual ~Stmt() = default;
    virtual Stmt* clone() const = 0;

    /**
     * \brief Internal pretty-printer which can suppress the output of the stdlib
     *
     * \param ctx Whether the current associative context is ambiguous
     */
    virtual std::ostream& pretty_print(std::ostream& os, bool suppress_std) const = 0;
    std::ostream& pretty_print(std::ostream& os) const override {
      return pretty_print(os, false);
    }
  };

  /**
   * \class synthewareQ::ast::MeasureStmt
   * \brief Class for measurement statements
   * \see synthewareQ::ast::Stmt
   */
  class MeasureStmt final : public Stmt {
    VarAccess q_arg_; ///< the quantum bit|register
    VarAccess c_arg_; ///< the classical bit|register

  public:
    MeasureStmt(parser::Position pos, VarAccess&& q_arg, VarAccess&& c_arg)
      : Stmt(pos)
      , q_arg_(std::move(q_arg))
      , c_arg_(std::move(c_arg))
    {}

    VarAccess& q_arg() { return q_arg_; }
    VarAccess& c_arg() { return c_arg_; }

    void set_qarg(const VarAccess& arg) { q_arg_ = arg; }
    void set_carg(const VarAccess& arg) { c_arg_ = arg; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "measure " << q_arg_ << " -> " << c_arg_ << ";\n";
      return os;
    }
    MeasureStmt* clone() const override {
      return new MeasureStmt(pos_, VarAccess(q_arg_), VarAccess(c_arg_));
    }
  };

  /**
   * \class synthewareQ::ast::ResetStmt
   * \brief Class for reset statements
   * \see synthewareQ::ast::Stmt
   */
  class ResetStmt final : public Stmt {
    VarAccess arg_; ///< the qbit|qreg

  public:
    ResetStmt(parser::Position pos, VarAccess&& arg) : Stmt(pos), arg_(std::move(arg)) {}

    VarAccess& arg() { return arg_; }
    void set_arg(const VarAccess& arg) { arg_ = arg; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "reset " << arg_ << ";\n";
      return os;
    }
    ResetStmt* clone() const override {
      return new ResetStmt(pos_, VarAccess(arg_));
    }
  };

  /**
   * \class synthewareQ::ast::IfStmt
   * \brief Class for if statements
   * \see synthewareQ::ast::Stmt
   */
  class IfStmt final : public Stmt {
    symbol var_;     ///< classical register name
    int cond_;       ///< value to check against
    ptr<Stmt> then_; ///< statement to be executed if true

  public:
    IfStmt(parser::Position pos, symbol var, int cond, ptr<Stmt> then)
      : Stmt(pos)
      , var_(var)
      , cond_(cond)
      , then_(std::move(then))
    {}

    const symbol& var() const { return var_; }
    int cond() const { return cond_; }
    Stmt& then() { return *then_; }
    void set_then(ptr<Stmt> then) { then_ = std::move(then); }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "if (" << var_ << "==" << cond_ << ") " << *then_;
      return os;
    }
    IfStmt* clone() const override {
      return new IfStmt(pos_, var_, cond_, ptr<Stmt>(then_->clone()));
    }
  };

  /**
   * \class synthewareQ::ast::Gate
   * \brief Statement sub-class for gate
   */
  class Gate : public Stmt {
  public:
    Gate(parser::Position pos) : Stmt(pos) {}
    virtual ~Gate() = default;
    virtual Gate* clone() const = 0;
  };

  /**
   * \class synthewareQ::ast::UGate
   * \brief Class for U gates
   * \see synthewareQ::ast::Gate
   */
  class UGate final : public Gate {
    ptr<Expr> theta_;  ///< theta angle
    ptr<Expr> phi_;    ///< phi angle
    ptr<Expr> lambda_; ///< lambda angle

    VarAccess arg_;    ///< quantum bit|register

  public:
    UGate(parser::Position pos, ptr<Expr> theta, ptr<Expr> phi, ptr<Expr> lambda, VarAccess&& arg)
      : Gate(pos)
      , theta_(std::move(theta))
      , phi_(std::move(phi))
      , lambda_(std::move(lambda))
      , arg_(std::move(arg))
    {}

    Expr& theta() { return *theta_; }
    Expr& phi() { return *phi_; }
    Expr& lambda() { return *lambda_; }
    VarAccess& arg() { return arg_; }

    void set_theta(ptr<Expr> theta) { theta_ = std::move(theta); }
    void set_phi(ptr<Expr> phi) { phi_ = std::move(phi); }
    void set_lambda(ptr<Expr> lambda) { lambda_ = std::move(lambda); }
    void set_arg(const VarAccess& arg) { arg_ = arg; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "U(" << *theta_ << "," << *phi_ << "," << *lambda_ << ") " << arg_ << ";\n";
      return os;
    }
    UGate* clone() const override {
      return new UGate(pos_,
                       ptr<Expr>(theta_->clone()),
                       ptr<Expr>(phi_->clone()),
                       ptr<Expr>(lambda_->clone()),
                       VarAccess(arg_));
    }
  };

  /**
   * \class synthewareQ::ast::CNOTGate
   * \brief Class for CX gates
   * \see synthewareQ::ast::Gate
   */
  class CNOTGate final : public Gate {
    VarAccess ctrl_; ///< control qubit|qreg
    VarAccess tgt_;  ///< target qubit|qreg

  public:
    CNOTGate(parser::Position pos, VarAccess&& ctrl, VarAccess&& tgt)
      : Gate(pos)
      , ctrl_(std::move(ctrl))
      , tgt_(std::move(tgt))
    {}

    VarAccess& ctrl() { return ctrl_; }
    VarAccess& tgt() { return tgt_; }

    void set_ctrl(const VarAccess& ctrl) { ctrl_ = ctrl; }
    void set_tgt(const VarAccess& tgt) { tgt_ = tgt; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "CX " << ctrl_ << "," << tgt_ << ";\n";
      return os;
    }
    CNOTGate* clone() const override {
      return new CNOTGate(pos_, VarAccess(ctrl_), VarAccess(tgt_));
    }
  };

  /**
   * \class synthewareQ::ast::BarrierGate
   * \brief Class for barrier gates
   * \see synthewareQ::ast::Gate
   */
  class BarrierGate final : public Gate {
    std::vector<VarAccess> args_; ///< list of quantum bits|registers

  public:
    BarrierGate(parser::Position pos, std::vector<VarAccess>&& args)
      : Gate(pos)
      , args_(std::move(args))
      {}

    int num_args() const { return args_.size(); }
    std::vector<VarAccess>& args() { return args_; }
    VarAccess& arg(int i) { return args_[i]; }
    void foreach_arg(std::function<void(VarAccess&)> f) {
      for (auto it = args_.begin(); it != args_.end(); it++) f(*it);
    }

    void set_arg(int i, const VarAccess& arg) { args_[i] = arg; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "barrier ";
      for (auto it = args_.begin(); it != args_.end(); it++) {
        os << (it == args_.begin() ? "" : ",") << *it;
      }
      os << ";\n";
      return os;
    }
    BarrierGate* clone() const override {
      return new BarrierGate(pos_, std::vector<VarAccess>(args_));
    }
  };

  /**
   * \class synthewareQ::ast::DeclaredGate
   * \brief Class for declared gate applications
   * \see synthewareQ::ast::Gate
   */
  class DeclaredGate final : public Gate {
    symbol name_;                    ///< gate identifier
    std::vector<ptr<Expr> > c_args_; ///< list of classical arguments
    std::vector<VarAccess> q_args_;  ///< list of quantum arguments

  public:
    DeclaredGate(parser::Position pos,
                 symbol name,
                 std::vector<ptr<Expr> >&& c_args,
                 std::vector<VarAccess>&& q_args)
      : Gate(pos)
      , name_(name)
      , c_args_(std::move(c_args))
      , q_args_(std::move(q_args))
    {}

    const symbol& name() const { return name_; }
    int num_cargs() const { return c_args_.size(); }
    int num_qargs() const { return q_args_.size(); }
    Expr& carg(int i) { return *(c_args_[i]); }
    VarAccess& qarg(int i) { return q_args_[i]; }
    std::vector<VarAccess>& qargs() { return q_args_; }
    void foreach_carg(std::function<void(Expr&)> f) {
      for (auto it = c_args_.begin(); it != c_args_.end(); it++) f(**it);
    }
    void foreach_qarg(std::function<void(VarAccess&)> f) {
      for (auto it = q_args_.begin(); it != q_args_.end(); it++) f(*it);
    }

    void set_carg(int i, ptr<Expr> expr) { c_args_[i] = std::move(expr); }
    void set_qarg(int i, const VarAccess& arg) { q_args_[i] = arg; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << name_;
      if (c_args_.size() > 0) {
        os << "(";
        for (auto it = c_args_.begin(); it != c_args_.end(); it++) {
          os << (it == c_args_.begin() ? "" : ",") << **it;
        }
        os << ")";
      }
      os << " ";
      for (auto it = q_args_.begin(); it != q_args_.end(); it++) {
        os << (it == q_args_.begin() ? "" : ",") << *it;
      }
      os << ";\n";
      return os;
    }
    DeclaredGate* clone() const override {
      std::vector<ptr<Expr> > c_tmp;
      for (auto it = c_args_.begin(); it != c_args_.end(); it++) {
        c_tmp.emplace_back(ptr<Expr>((*it)->clone()));
      }

      return new DeclaredGate(pos_, name_, std::move(c_tmp), std::vector<VarAccess>(q_args_));
    }
  };

}
}
