/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"
#include "qasm/visitors/source_printer.hpp"   // For qelib identifiers

#include "substitution.hpp"

#include <unordered_map>

namespace synthewareQ {
namespace transformations {

  // TODO: hoist ancilla declarations

  using namespace qasm;

  /* \brief! Inlines gate calls
   *
   * Traverses an AST and inlines all gate calls. By default qelib calls are NOT
   * inlined, but optionally can be. Local ancillas are hoisted to the global
   * level and reused
   */
  void inline_ast(ast_context*);

  /* Implementation */
  class inliner final : public replacer<inliner> {
  public:
    using replacer<inliner>::visit;

    struct config {
      bool inline_qelib = false;
      bool keep_declarations = true;
    };

    inliner(ast_context* ctx) : ctx_(ctx), substitutor_(ctx) {}
    inliner(ast_context* ctx, const config& params) : ctx_(ctx), substitutor_(ctx), config_(params) {}

    std::optional<ast_node_list> replace(decl_gate* node) override {
      // Replacement is post-order, so body should be inlined now

      if (node->has_body()) {
        auto& tmp = gate_decls_[node->identifier()];
        tmp.c_params = node->has_parameters() ? static_cast<list_ids*>(&node->parameters()) : nullptr;
        tmp.q_params = static_cast<list_ids*>(&node->arguments());
        tmp.body     = &node->body();

        if (config_.keep_declarations) {
          return std::nullopt;
        } else {
          return ast_node_list({});
        }
      } else {
        // Opaque decl, don't inline
        return std::nullopt;
      }
    }

    std::optional<ast_node_list> replace(stmt_gate* node) override {
      if (!config_.inline_qelib && qelib_defs.find(node->gate()) != qelib_defs.end()) {
        return std::nullopt;
      }
      
      if (auto it = gate_decls_.find(node->gate()); it != gate_decls_.end()) {
        ast_node_list ret;
        ast_node* body = it->second.body->copy(ctx_);

        // Generate a subst map
        std::unordered_map<std::string_view, ast_node*> substs;
        if (node->has_cargs()) {
          auto c_args = static_cast<list_exprs*>(&node->c_args());
          auto p_it = it->second.c_params->begin();
          auto a_it = c_args->begin();
          for (; (p_it != it->second.c_params->end()) && (a_it != c_args->end()); p_it++, a_it++) {
            substs[(static_cast<expr_var&>(*p_it)).id()] = &(*a_it);
          }
        }

        auto q_args = static_cast<list_aps*>(&node->q_args());
        auto p_it = it->second.q_params->begin();
        auto a_it = q_args->begin();
        for (; (p_it != it->second.q_params->end()) && (a_it != q_args->end()); p_it++, a_it++) {
          substs[(static_cast<expr_var&>(*p_it)).id()] = &(*a_it);
        }

        // Perform the substitution
        substitutor_.subst(substs, body);
        
        ret.push_back(&node->parent(), body);
        return ret;
      } else {
        return std::nullopt;
      }
    }

  private:
    struct gate_info {
      list_ids* c_params;
      list_ids* q_params;
      ast_node* body;
    };
    
    ast_context* ctx_;
    config config_;
    std::unordered_map<std::string_view, gate_info> gate_decls_;
    substitutor substitutor_;
  };

  void inline_ast(ast_context* ctx) {
    auto tmp = inliner(ctx);
    tmp.visit(*ctx);
  }

  void inline_ast(ast_context* ctx, const inliner::config& params) {
    auto tmp = inliner(ctx, params);
    tmp.visit(*ctx);
  }

}
}
