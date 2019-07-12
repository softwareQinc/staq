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

  using namespace qasm;
  using namespace transformations;

  /* \brief! Applies a layout to a circuit */
  class layout_transformer final : public replacer<layout_transformer> {
  public:
    using replacer<layout_transformer>::visit;

    struct config {
      std::string register_name = "q";
    };

    layout_transformer(ast_context* ctx, layout& l) : ctx_(ctx), layout_(l), subst_(ctx) {}

    std::optional<ast_node_list> replace(decl_program* node) override {
      // Visit entire program, removing register declarations, then
      // add the physical register & apply substitutions

      // Physical register declaration
      auto reg = decl_register::build(ctx_, node->location(), register_type::quantum,
                                      config_.register_name, layout_.size());
      node->insert_child(node->begin(), reg);

      // Substitution
      std::map<ap, ap> substitution;
      for (auto& [access, offset] : layout_) {
        substitution[access] = ap(std::make_pair(config_.register_name, offset));
      }
      subst_.subst(substitution, node);

      return std::nullopt;
    }

    std::optional<ast_node_list> replace(decl_register* node) override {
      if (node->is_quantum()) return ast_node_list();
      else return std::nullopt;
    }

  private:
    ast_context* ctx_;

    layout layout_;
    ap_substitutor subst_;
    config config_;

  };

  /* \brief! A basic qubit layout algorithm */
  class basic_layout final : public visitor<basic_layout> {
  public:
    using visitor<basic_layout>::visit;

    basic_layout(ast_context* ctx, device& d) : ctx_(ctx), device_(d) {}

    layout generate() {
      current_ = layout();
      n_ = 0;

      visit(*ctx_);

      return current_;
    }

    void visit(decl_register* node) override {
      if (node->is_quantum()) {
        if (n_ + node->size() <= device_.qubits_) {
          for (auto i = 0; i < node->size(); i++) {
            current_[std::make_pair(node->identifier(), i)] = n_ + i;
          }
          n_ += node->size();
        } else {
          std::cerr << "Error: program can't fit on device \"" << device_.name_ << "\"\n";
        }
      }
    }

  private:
    ast_context* ctx_;
    
    device device_;
    layout current_;
    size_t n_;

  };

  void apply_layout(ast_context* ctx, layout& l) {
    layout_transformer trans(ctx, l);
    trans.visit(*ctx);
  }

  layout compute_layout(ast_context* ctx, device& d) {
    basic_layout layout_generator(ctx, d);
    return layout_generator.generate();
  }

}
}
