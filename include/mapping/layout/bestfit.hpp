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

namespace synthewareQ {
namespace mapping {

  using namespace qasm;
  using namespace transformations;

  using ap = std::pair<std::string_view, uint32_t>;

  /* \brief! An initial layout based on the distribution of connections in the circuit
   *
   * Chooses a layout where the most coupled virtual qubits are assigned to the highest
   * fidelity couplings. Should perform well for devices with a high degree of connectivity
   */
  class best_fit final : public visitor<best_fit> {
  public:
    using visitor<best_fit>::visit;

    best_fit(ast_context* ctx, device& d) : ctx_(ctx), device_(d) {}

    layout generate() {
      allocated_ = std::vector<bool>(device_.qubits_, false);
      access_paths_.clear();
      histogram_.clear();  

      visit(*ctx_);

      return fit_histogram();
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

    void visit_pre(stmt_cnot* node) override {
      auto ctrl = get_access_path(&node->control());
      auto tgt = get_access_path(&node->target());

      histogram_[std::make_pair(ctrl, tgt)] += 1;
    }

  private:
    ast_context* ctx_;
    
    device device_;
    std::vector<bool> allocated_;
    std::set<ap> access_paths_;
    std::map<std::pair<ap, ap>, uint32_t> histogram_;

    layout fit_histogram() {
      layout ret;

      // Sort in order of decreasing number of two-qubit gates
      using mapping = std::pair<std::pair<ap, ap>, uint32_t>;
      using comparator = std::function<bool(mapping, mapping)>;
      comparator cmp = [](mapping a, mapping b) { return a.second > b.second; };
      std::set<mapping,comparator> sorted_pairs(histogram_.begin(), histogram_.end(), cmp);

      // For each pair with CNOT gates between them, try to assign a coupling
      auto couplings = device_.couplings();
      for (auto& [args, val] : sorted_pairs) {
        size_t ctrl_bit;
        size_t tgt_bit;
        for (auto& [coupling, f] : couplings) {
          if (auto it = ret.find(args.first); it != ret.end()) {
            if (it->second != coupling.first) continue;
            else ctrl_bit = it->second;
          } else if (!allocated_[coupling.first]) {
            ctrl_bit = coupling.first;
          } else {
            continue;
          }

          if (auto it = ret.find(args.second); it != ret.end()) {
            if (it->second != coupling.second) continue;
            else tgt_bit = it->second;
          } else if (!allocated_[coupling.second]) {
            tgt_bit = coupling.second;
          } else {
            continue;
          }

          ret[args.first] = ctrl_bit;
          ret[args.second] = tgt_bit;
          allocated_[ctrl_bit] = true;
          allocated_[tgt_bit] = true;
          couplings.erase(std::make_pair(coupling, f));
          break;
        }
      }

      // For any remaining access paths, map them
      for (auto ap : access_paths_) {
        auto i = 0;
        bool cont = ret.find(ap) == ret.end();
        while (cont) {
          if (i >= device_.qubits_) {
            std::cerr << "Error: ran out of physical qubits to allocate";
            return ret;
          } else if (!allocated_[i]) {
              ret[ap] = i;
              allocated_[i] = true;
              cont = false;
          }

          i++;
        }
      }

      return ret;
    }
        

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

  layout compute_layout_bestfit(ast_context* ctx, device& d) {
    best_fit generator(ctx, d);
    return generator.generate();
  }

}
}
