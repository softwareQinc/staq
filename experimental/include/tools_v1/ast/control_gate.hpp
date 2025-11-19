/*
 * This file is part of tools_v1.
 *
 * Copyright (c) 2019 - 2025 softwareQ Inc. All rights reserved.
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
 * \file tools_v1/ast/control_gate.hpp
 * \brief Control gate implementation
 */

#ifndef TOOLS_V1_AST_CONTROL_GATE_HPP_
#define TOOLS_V1_AST_CONTROL_GATE_HPP_

#include "stmt.hpp"
#include <vector>

namespace tools_v1 {
namespace ast {

/**
 * \class tools_v1::ast::ControlGate
 * \brief Class for controlled gates
 * \see tools_v1::ast::Gate
 */
class ControlGate final : public Gate {
  VarAccess ctrl_;        ///< control qubit
  ptr<Gate> target_gate_; ///< gate to be controlled

public:
  /**
   * \brief Constructs a control gate
   *
   * \param pos The source position
   * \param ctrl Rvalue reference to the control qubit
   * \param target_gate Rvalue reference to the target gate
   */
  ControlGate(parser::Position pos, VarAccess &&ctrl, ptr<Gate> &&target_gate)
      : Gate(pos), ctrl_(std::move(ctrl)),
        target_gate_(std::move(target_gate)) {}

  /**
   * \brief Protected heap-allocated construction
   */
  static ptr<ControlGate> create(parser::Position pos, VarAccess &&ctrl,
                                 ptr<Gate> &&target_gate) {
    return std::make_unique<ControlGate>(pos, std::move(ctrl),
                                         std::move(target_gate));
  }

  /**
   * \brief Get the control qubit
   *
   * \return Reference to the control qubit
   */
  VarAccess &ctrl() { return ctrl_; }

  /**
   * \brief Get the target gate
   *
   * \return Reference to the target gate
   */
  Gate &target_gate() { return *target_gate_; }

  /**
   * \brief Set the control qubit
   *
   * \param ctrl The new control qubit
   */
  void set_ctrl(const VarAccess &ctrl) { ctrl_ = ctrl; }

  /**
   * \brief Set the target gate
   *
   * \param target_gate The new target gate
   */
  void set_target_gate(ptr<Gate> target_gate) {
    target_gate_ = std::move(target_gate);
  }

  void accept(Visitor &visitor) override { visitor.visit(*this); }
  std::ostream &pretty_print(std::ostream &os,
                             bool suppress_std) const override {
    os << "control " << ctrl_ << " :: " << *target_gate_;
    return os;
  }

protected:
  ControlGate *clone() const override {
    return new ControlGate(pos_, VarAccess(ctrl_),
                           object::clone(*target_gate_));
  }
};

class MultiControlGate final : public Gate {
  symbol name_ = "MultiControlGate";
  std::vector<VarAccess> ctrl_1_; ///< control qubit
  std::vector<VarAccess> ctrl_2_; ///< control qubit
  ptr<Gate> target_gate_;         ///< gate to be controlled

public:
  /**
   * \brief Constructs a control gate
   *
   * \param pos The source position
   * \param ctrl Rvalue reference to the control qubit
   * \param target_gate Rvalue reference to the target gate
   */
  MultiControlGate(parser::Position pos, std::vector<VarAccess> &&ctrl1,
                   std::vector<VarAccess> &&ctrl2, ptr<Gate> &&target_gate)
      : Gate(pos), ctrl_1_(std::move(ctrl1)), ctrl_2_(std::move(ctrl2)),
        target_gate_(std::move(target_gate)) {}

  /**
   * \brief Protected heap-allocated construction
   */
  static ptr<MultiControlGate> create(parser::Position pos,
                                      std::vector<VarAccess> &&ctrl1,
                                      std::vector<VarAccess> &&ctrl2,
                                      ptr<Gate> &&target_gate) {
    return std::make_unique<MultiControlGate>(
        pos, std::move(ctrl1), std::move(ctrl2), std::move(target_gate));
  }

  /**
   * \brief Get the control qubit
   *
   * \return Reference to the control qubit
   */
  std::vector<VarAccess> &ctrl1() { return ctrl_1_; }
  std::vector<VarAccess> &ctrl2() { return ctrl_2_; }

  /**
   * \brief Get the target gate
   *
   * \return Reference to the target gate
   */
  Gate &target_gate() { return *target_gate_; }

  /**
   * \brief Set the control qubit
   *
   * \param ctrl The new control qubit
   */
  void set_ctrl_1(const std::vector<VarAccess> &ctrl) { ctrl_1_ = ctrl; }
  void set_ctrl_2(const std::vector<VarAccess> &ctrl) { ctrl_2_ = ctrl; }

  /**
   * \brief Set the target gate
   *
   * \param target_gate The new target gate
   */
  void set_target_gate(ptr<Gate> target_gate) {
    target_gate_ = std::move(target_gate);
  }

  std::string name(){ return name_; }

  void accept(Visitor &visitor) override { visitor.visit(*this); }
  std::ostream &pretty_print(std::ostream &os,
                             bool suppress_std) const override {
    os << "multicontrol [";
    if (ctrl_1_.size() > 0) {
      os << ctrl_1_[0];
    }
    for (int i = 1; i < ctrl_1_.size(); i++) {
      os << ", " << ctrl_1_[i];
    }
    os << "] :: [";
    if (ctrl_2_.size() > 0) {
      os << ctrl_2_[0];
    }
    for (int i = 1; i < ctrl_2_.size(); i++) {
      os << ", " << ctrl_2_[i];
    }
    os << "] :: ";
    os << *target_gate_;
    return os;
  }

protected:
  MultiControlGate *clone() const override {
    return new MultiControlGate(pos_, std::vector<VarAccess>(ctrl_1_),
                                std::vector<VarAccess>(ctrl_2_),
                                object::clone(*target_gate_));
  }
};

} /* namespace ast */
} /* namespace tools_v1 */

#endif /* TOOLS_V1_AST_CONTROL_GATE_HPP_ */
