/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/concrete.hpp"

#include "mapping/device.hpp"

#include <map>
#include <vector>
#include <set>

namespace synthewareQ {
namespace mapping {

  using namespace qasm;
  using namespace transformations;

  using ap = std::pair<std::string_view, uint32_t>;

  /* \brief! Allocates qubits on demand prioritizing coupling fidelity */
  class eager_layout final : public visitor<eager_layout> {
  public:
    using visitor<eager_layout>::visit;

    eager_layout(ast_context* ctx, device& d) : ctx_(ctx), device_(d) {
      couplings_ = device_.couplings();
    }

    layout generate() {
      layout_ = layout();
      allocated_ = std::vector<bool>(device_.qubits_, false);
      access_paths_.clear();

      visit(*ctx_);

      for (auto ap : access_paths_) {
        auto i = 0;
        bool cont = layout_.find(ap) == layout_.end();
        while (cont) {
          if (i >= device_.qubits_) {
            std::cerr << "Error: ran out of physical qubits to allocate";
            return layout_;
          } else if (!allocated_[i]) {
              layout_[ap] = i;
              allocated_[i] = true;
              cont = false;
          }

          i++;
        }
      }

      return layout_;
    }

    // Ignore gate declarations
    void visit(decl_gate* node) override { }

    void visit(decl_register* node) override {
      if (node->is_quantum()) {
        for (auto i = 0; i < node->size(); i++) {
          access_paths_.insert(std::make_pair(node->identifier(), i));
        }
      }
    }

    // Try to assign a coupling to the cnot
    void visit_pre(stmt_cnot* node) override {
      auto ctrl = get_access_path(&node->control());
      auto tgt = get_access_path(&node->target());

      if (layout_.find(ctrl) == layout_.end() && layout_.find(tgt) == layout_.end()) {
        return;
      }

      size_t ctrl_bit;
      size_t tgt_bit;
      for (auto& [coupling, f] : couplings_) {
        if (auto it = layout_.find(ctrl); it != layout_.end()) {
          if (it->second != coupling.first) break;
          else ctrl_bit = it->second;
        } else if (!allocated_[coupling.first]) {
          ctrl_bit = coupling.first;
        } else {
          break;
        }

        if (auto it = layout_.find(tgt); it != layout_.end()) {
          if (it->second != coupling.second) break;
          else tgt_bit = it->second;
        } else if (!allocated_[coupling.second]) {
          tgt_bit = coupling.first;
        } else {
          break;
        }

        layout_[ctrl] = ctrl_bit;
        layout_[tgt] = tgt_bit;
        allocated_[ctrl_bit] = true;
        allocated_[tgt_bit] = true;
      }

    }

  private:
    ast_context* ctx_;
    
    device device_;
    layout layout_;
    std::vector<bool> allocated_;
    std::set<ap> access_paths_;
    std::set<std::pair<coupling, double> > couplings_;

    std::pair<std::string_view, uint32_t> get_access_path(ast_node* node) {
      expr_reg_offset* arg = nullptr;
      switch(node->kind()) {
      case ast_node_kinds::expr_reg_offset:
        arg = static_cast<expr_reg_offset*>(node);
        return std::make_pair(arg->id(), arg->index_numeric());
      default:
        throw std::logic_error("Can't complete mapping -- argument invalid");
      }
    }

  };

  layout compute_layout_eager(ast_context* ctx, device& d) {
    eager_layout generator(ctx, d);
    return generator.generate();
  }

}
}
