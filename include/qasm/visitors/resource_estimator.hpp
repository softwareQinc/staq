/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "generic/base.hpp"
#include "source_printer.hpp"

#include <unordered_map>
#include <set>
#include <sstream>

// TODO: Account for compound gates, i.e. qreg q[n]; reset q;
// TODO: Depth computations. Requires dependence visitor

namespace synthewareQ {
namespace qasm {

  typedef std::unordered_map<std::string, uint32_t> resource_count;

  void add_counts(resource_count& A, const resource_count& B) {
    for (auto& [gate, num] : B) A[gate] += num;
  }

  class resource_estimator final : public visitor_base<resource_estimator> {
  public:
    using visitor_base<resource_estimator>::visit;

    struct config {
      config() : unbox(true), merge_dagger(true), overrides(qelib_defs) {}
      config(bool u, const std::set<std::string_view>& o) : unbox(u), overrides(o) {}

      bool unbox;
      bool merge_dagger;
      std::set<std::string_view> overrides;
    };

    resource_estimator() : visitor_base<resource_estimator>() {}
    resource_estimator(const config& params) : visitor_base<resource_estimator>() , config_(params) {}
    ~resource_estimator() {}

    resource_count estimate(ast_context& ctx) {
      visit(ctx);

      // Unboxing the running estimate
      auto& [counts, depths] = running_estimate_;

      // Get maximum critical path length
      uint32_t depth = 0;
      for (auto& [id, length] : depths) {
        if (length > depth) depth = length;
      }

      // Set depth and return
      counts["depth"] = depth;
      return counts;
    }

    /* Resoure counting for specific nodes */
    void visit(decl_program* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
    }
    void visit(decl_register* node) {
      auto& [counts, depths] = running_estimate_;

      if (node->is_quantum()) {
        counts["qubits"] += node->size();
      } else {
        counts["classical bits"] += node->size();
      }
    }
    void visit(decl_param* node) {}
    void visit(decl_gate* node) {
      // Initialize a new resource count
      auto& local_state = resource_map_[node->identifier()];
      std::swap(running_estimate_, local_state);

      for (auto& child : *node) visit(const_cast<ast_node*>(&child));

      std::swap(running_estimate_, local_state);
    }

    /* Statements */
    void visit(stmt_barrier* node) {
      auto& [counts, depths] = running_estimate_;
      counts["cx"] += 1;
    }
    void visit(stmt_cnot* node) {
      auto& [counts, depths] = running_estimate_;
      counts["cx"] += 1;
    }
    void visit(stmt_unitary* node) {
      auto& [counts, depths] = running_estimate_;

      stream_ << "u(";
      printer_.visit(&node->theta());
      stream_ << ",";
      printer_.visit(&node->phi());
      stream_ << ",";
      printer_.visit(&node->lambda());
      stream_ << ")";

      auto name = stream_.str();
      clear();

      counts[name] += 1;
    }
    void visit(stmt_gate* node) {
      // TODO: absorb parameters into gate name
      auto& [counts, depths] = running_estimate_;
      printer_.visit(&node->gate());
      auto name = stream_.str();
      clear();

      if (config_.merge_dagger) strip_dagger(name);

      if (config_.unbox && (config_.overrides.find(name) == config_.overrides.end())) {
        add_counts(counts, resource_map_[name].first);
      } else {
        counts[name] += 1;
      }

    }
    void visit(stmt_reset* node) {
      auto& [counts, depths] = running_estimate_;
      counts["reset"] += 1;
    }
    void visit(stmt_measure* node) {
      auto& [counts, depths] = running_estimate_;
      counts["measurement"] += 1;
    }
    void visit(stmt_if* node) {
      visit(&node->quantum_op());
    }

    /* Expressions */
    void visit(expr_decl_ref* node) {}
    void visit(expr_reg_idx_ref* node) {}
    void visit(expr_integer* node) {}
    void visit(expr_pi* node) {}
    void visit(expr_real* node) {}
    void visit(expr_binary_op* node) {}
    void visit(expr_unary_op* node) {}

    /* Extensions */
    void visit(decl_oracle*) {}
    void visit(decl_ancilla*) {}

    /* Lists */
    void visit(list_gops* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
    }
    void visit(list_ids* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
    }
      

  private:
    using depth_count = std::unordered_map<std::string, uint32_t>;
    using resource_state = std::pair<resource_count, depth_count>;

    config config_;
    std::unordered_map<std::string_view, resource_state> resource_map_;

    resource_state running_estimate_;

    // String stream and source printer for getting string representations of arguments
    // The use of the source printer is overkill, but it's the easiest and safest way
    std::stringstream stream_;
    source_printer printer_ = source_printer(stream_);

    void clear() { stream_.str(std::string()); }

    void strip_dagger(std::string& str) {
      auto len = str.size();
      
      if (len > 2 && str[len - 2] == 'd' && str[len - 1] == 'g') {
        str.resize(len-2);
      }
    }
  };
}
}
