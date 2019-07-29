/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"
#include "qasm/visitors/generic/concrete.hpp"
#include "transformations/substitution.hpp"
#include "synthesis/cnot_dihedral.hpp"
#include "synthesis/logic_synthesis.hpp" // For angle to expr
#include "mapping/device.hpp"


#include <map>

namespace synthewareQ {
namespace mapping {

  /*! \brief Steiner tree based resynthesizing mapper
   *
   * Resynthesizes an entire circuit by breaking into cnot-dihedral "chunks"
   * and resynthesizing those using gray-synth (arXiv:1712.01859) extended
   * with a device dependent mapping technique based on Steiner trees 
   * (arXiv:1904.01972)
   *
   * Assumes the circuit has already been laid out onto a single register 
   * with name given in the configuration
   */
  void steiner_mapping(ast_context* ctx, device d);

  /* Implementation */
  class steiner_mapper final : public replacer<steiner_mapper> {
  public:
    using replacer<steiner_mapper>::visit;

    struct config {
      std::string register_name = "q";
    };

    // Ignore declarations if they were left in during inlining
    void visit(decl_gate* node) override {}
    void visit(decl_oracle* node) override {}

    std::optional<ast_node_list> replace(stmt_cnot* node) override {
      assert(node->control().kind() == ast_node_kinds::expr_reg_offset);
      assert(node->target().kind() == ast_node_kinds::expr_reg_offset);
      auto ctrl = static_cast<expr_reg_offset*>(&node->control());
      auto trgt = static_cast<expr_reg_offset*>(&node->target());

      assert(ctrl->index().kind() == ast_node_kinds::expr_integer);
      assert(trgt->index().kind() == ast_node_kinds::expr_integer);
      auto ctl = ctrl->index_numeric();
      auto tgt = trgt->index_numeric();

      if (in_bounds(ctl) && in_bounds(tgt)) {
        permutation_[tgt] ^= permutation_[ctl];
      } else {
        throw std::logic_error("CNOT argument(s) out of device bounds!");
      }

      // Delete the gate
      return ast_node_list();
    }

    std::optional<ast_node_list> replace(stmt_unitary* node) override {
      if (is_zero(&node->theta()) && is_zero(&node->phi())) {
        // It's a z-axis rotation
        auto angle = expr_to_angle(&node->lambda());
        auto idx = get_index(&node->arg());

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else {
        return flush(node);
      }
    }
    std::optional<ast_node_list> replace(stmt_gate* node) override {
      auto name = node->gate();
      auto cargs = static_cast<expr_list>(node->c_args());
      auto qargs = static_cast<ap_list>(node->q_args());

      if (name == "rz" || name == "u1") {
        auto angle = expr_to_angle(&(*cargs.begin()));
        auto idx = get_index(&(*qargs.begin()));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else if (name == "z") {
        auto angle = td::angles::pi;
        auto idx = get_index(&(*qargs.begin()));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else if (name == "s") {
        auto angle = td::angles::pi_half;
        auto idx = get_index(&(*qargs.begin()));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else if (name == "sdg") {
        auto angle = -td::angles::pi_half;
        auto idx = get_index(&(*qargs.begin()));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else if (name == "t") {
        auto angle = td::angles::pi_quarter;
        auto idx = get_index(&(*qargs.begin()));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else if (name == "tdg") {
        auto angle = -td::angles::pi_quarter;
        auto idx = get_index(&(*qargs.begin()));

        if (in_bounds(idx)) {
          add_phase(permutation_[idx], angle);
        } else {
          throw std::logic_error("Unitary argument out of device bounds!");
        }

        return ast_node_list();
      } else {
        return flush(node);
      }
    }

    // Always generate a synthesis event
    std::optional<ast_node_list> replace(stmt_if* node) override { return flush(node); }
    std::optional<ast_node_list> replace(stmt_barrier* node) override { return flush(node); }
    std::optional<ast_node_list> replace(stmt_measure* node) override { return flush(node); }
    std::optional<ast_node_list> replace(stmt_reset* node) override { return flush(node); }

  private:
    ast_context* ctx_;

    device device_;
    config config_;

    // Accumulating data
    std::list<synthesis::phase_term> phases_;
    synthesis::linear_op<bool> permutation_;

    // NOTE: this algorithm does NOT do any phase merging. That should be handled by the rotation
    // merging optimization
    void add_phase(vector<bool> parity, td::angle angle) {
      phases_.push_back(std::make_pair(parity, angle));
    }

    // Flushes a cnot-dihedral operator (i.e. phases + permutation) to the circuit
    // before the given node
    ast_node_list flush(ast_node* node) {
      ast_node_list ret;
      
      // Synthesize circuit
      for (auto gate : gray_steiner(phases_, permutation_, device_)) {
        std::visit(overloaded {
            [&ret](std::pair<size_t, size_t> cx) {
              if (device.coupled(cx.first, cx.second)) {
                ret.push_back(generate_cnot(cx.first, cs.second));
              } else if (device.couple(cx.second, cx.first)) {
                ret.push_back(generate_hadamard(cx.first));
                ret.push_back(generate_hadamard(cx.second));
                ret.push_back(generate_cnot(cx.first, cs.second));
                ret.push_back(generate_hadamard(cx.first));
                ret.push_back(generate_hadamard(cx.second));
              } else {
                throw std::logic_error("CNOT between non-coupled vertices!");
              }
            },
            [&ret](std::pair<td::angle, size_t> rz) {
              ret.push_back(generate_rz(rz.first, rz.second));
            }}, gate);
      }
      ret.push_back(node->copy());

      // Reset the cnot-dihedral circuit
      phases_.clear();
      for (auto i = 0; i < device_.qubits_; i++) {
        for (auto j = 0; j < device_.qubits_; j++) {
          permutation_[i][j] = i == j ? true : false;
        }
      }

      return ret;
    }

    bool in_bounds(size_t i) { return 0 <= i && i < device_.qubits_; }

    bool is_zero(ast_node* node) {
      switch(node->kind()) {
      case ast_node_kinds::integer:
        return (static_cast<expr_integer*>(node))->evaluate() == 0;
      case ast_node_kinds::real:
        return (static_cast<expr_real*>(node))->evaluate() == 0;
      default:
        return false;
      }
    }

    uint32_t get_index(ast_node* node) {
      switch(node->kind()) {
      case ast_node_kinds::expr_reg_offset:
        return (static_cast<expr_reg_offset*>(node))->index_numeric();
      default:
        throw std::logic_error("Gate argument is not a register dereference!");
      }
    }

    // TODO: Find a better place for this
    double expr_to_angle(ast_node* node) {
      switch(node->kind()) {
      case ast_node_kinds::expr_pi:
        return M_PI;
      case ast_node_kinds::expr_integer:
        return (static_cast<expr_integer*>(node))->evaluate();
      case ast_node_kinds::expr_real:
        return (static_cast<expr_real*>(node))->evaluate();
      case ast_node_kinds::expr_binary_op: {
        auto bop = static_cast<expr_binary_op*>(node);
        auto lhs = expr_to_angle(&bop->left());
        auto rhs = expr_to_angle(&bop->right());

        switch(bop->op()) {
        case binary_ops::addition:
          return lhs + rhs;
        case binary_ops::subtraction:
          return lhs - rhs;
        case binary_ops::division:
          return lhs / rhs;
        case binary_ops::multiplication:
          return lhs * rhs;
        case binary_ops::exponentiation:
          return std::pow(lhs, rhs);
        case binary_ops::equality:
          return lhs == rhs;
        default:
          throw std::logic_error("Unknown binary operator");
        }
      }
      case ast_node_kinds::expr_unary_op: {
        auto uop = static_cast<expr_unary_op*>(node);
        auto lhs = expr_to_angle(&uop->subexpr());

        switch(uop->op()) {
        case unary_ops::sin:
          return sin(lhs);
        case unary_ops::cos:
          return cos(lhs);
        case unary_ops::tan:
          return tan(lhs);
        case unary_ops::exp:
          return exp(lhs);
        case unary_ops::ln:
          return log(lhs);
        case unary_ops::sqrt:
          return sqrt(lhs);
        case unary_ops::minus:
          return -lhs;
        case unary_ops::plus:
          return lhs;
        default:
          throw std::logic_error("Unknown unary operator");
        }
      }
      default:
        throw::logic_error("Could not generate angle from expression!");
      }
    }

    // Gate generation
    ast_node* generate_cnot(size_t i, size_t j, uint32_t loc = 0) {
      auto builder = stmt_cnot::builder(ctx_, loc);
      auto ctrl_offset = expr_integer::create(ctx_, loc, i);
      auto trgt_offset = expr_integer::create(ctx_, loc, j);

      builder.add_child(expr_reg_offset::build(ctx_, loc, config_.register_name, ctrl_offset));
      builder.add_child(expr_reg_offset::build(ctx_, loc, config_.register_name, trgt_offset));

      return static_cast<ast_node*>(builder.finish());
    }
      
    ast_node* generate_hadamard(size_t i, uint32_t loc = 0) {
      auto builder = stmt_unitary::builder(ctx_, loc);

      auto theta = expr_binary_op::builder(ctx_, loc, binary_ops::division);
      theta.add_child(expr_pi::create(ctx_, loc));
      theta.add_child(expr_integer::create(ctx_, loc, 2));
      auto phi = expr_integer::create(ctx_, loc, 0);
      auto lambda = expr_pi::create(ctx_, loc);

      builder.add_child(theta.finish());
      builder.add_child(phi);
      builder.add_child(lambda);

      auto offset = expr_integer::create(ctx_, loc, i);
      builder.add_child(expr_reg_offset::build(ctx_, loc, config_.register_name, offset));

      return static_cast<ast_node*>(builder.finish());
    }

    ast_node* generate_rz(td::angle theta, size_t i, uint32_t loc = 0) {
      auto builder = stmt_unitary::builder(ctx_, loc);

      auto theta = expr_integer::create(ctx_, loc, 0);
      auto phi = expr_integer::create(ctx_, loc, 0);
      auto lambda = synthewareQ::angle_to_expr(ctx_, loc, theta);

      builder.add_child(theta.finish());
      builder.add_child(phi);
      builder.add_child(lambda);

      auto offset = expr_integer::create(ctx_, loc, i);
      builder.add_child(expr_reg_offset::build(ctx_, loc, config_.register_name, offset));

      return static_cast<ast_node*>(builder.finish());
    }
  };
    
  void steiner_mapping(ast_context* ctx, device d) {
    steiner_mapper mapper(ctx, d);
    mapper.visit(*ctx);
  }
}
}
