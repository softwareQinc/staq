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

#include "mapping/device.hpp"

#include <map>

namespace synthewareQ {
namespace mapping {

  /* \brief! Simple swap-inserting mapping algorithm 
   *
   * Assumes the circuit has already been laid out onto a single register 
   * with name given in the configuration
   */
  void map_onto_device(ast_context* ctx, device d);

  /* Implementation */
  class swap_mapper final : public replacer<swap_mapper> {
  public:
    using replacer<swap_mapper>::visit;

    struct config {
      std::string register_name = "q";
    };

    swap_mapper(ast_context* ctx, device& d) : ctx_(ctx), device_(d) {
      for (auto i = 0; i < d.qubits_; i++) {
        permutation_[i] = i;
      }
    }

    std::optional<ast_node_list> replace(expr_reg_offset* node) override {
      if (node->id() == config_.register_name) {
        ast_node_list ret;

        auto tmp = node->index_numeric();
        auto new_offset = expr_integer::create(ctx_, node->location(), permutation_[tmp]);
        ret.push_back(&node->parent(), expr_reg_offset::build(ctx_, node->location(), node->id(), new_offset));

        return ret;
      }

      return std::nullopt;
    }

    // Where the magic happens
    std::optional<ast_node_list> replace(stmt_cnot* node) override {
      // Unsafe
      auto ctrl = static_cast<expr_reg_offset*>(&node->control());
      auto trgt = static_cast<expr_reg_offset*>(&node->target());

      // Post-order replacement, so the indices should already refer to the current permutation
      auto ctl = ctrl->index_numeric();
      auto tgt = trgt->index_numeric();
      path cnot_chain = device_.shortest_path(ctl, tgt);
      if (cnot_chain.empty()) {
        std::cerr << "Error: could not find path between qubits " << ctl << " and " << tgt << "\n";
        return std::nullopt;
      } else if (cnot_chain.front() == tgt) {
        return std::nullopt;
      } else {
        ast_node_list ret;
        // Create a swap chain & update the current permutation
        auto i = ctl;
        for (auto j : cnot_chain) {
          if (j == tgt) {
            ret.push_back(&node->parent(), generate_cnot(i, j, node->location()));
            break;
          } else {
            // Swap i and j
            auto swap_i = i;
            auto swap_j = j;
            if (!device_.coupled(i, j)) {
              swap_i = j;
              swap_j = i;
            }
            
            // CNOT 1
            ret.push_back(&node->parent(), generate_cnot(swap_i, swap_j, node->location()));

            // CNOT 2
            if (device_.coupled(swap_j, swap_i)) {
              ret.push_back(&node->parent(), generate_cnot(swap_j, swap_i, node->location()));
            } else {
              ret.push_back(&node->parent(), generate_hadamard(swap_i, node->location()));
              ret.push_back(&node->parent(), generate_hadamard(swap_j, node->location()));
              ret.push_back(&node->parent(), generate_cnot(swap_i, swap_j, node->location()));
              ret.push_back(&node->parent(), generate_hadamard(swap_i, node->location()));
              ret.push_back(&node->parent(), generate_hadamard(swap_j, node->location()));
            }

            // CNOT 3
            ret.push_back(&node->parent(), generate_cnot(swap_i, swap_j, node->location()));

            // Adjust permutation
            for (auto& [q_init, q] : permutation_) {
              if (q == i) q = j;
              else if (q == j) q = i;
            }
          }
          i = j;
        }
        return ret;
      }
    }

  private:
    ast_context* ctx_;

    device device_;
    std::map<size_t, size_t> permutation_;
    config config_;

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
  };

  void map_onto_device(ast_context* ctx, device d) {
    swap_mapper mapper(ctx, d);
    mapper.visit(*ctx);
  }

}
}
