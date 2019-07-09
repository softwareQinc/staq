/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"

#include "substitution.hpp"

#include <unordered_map>

namespace synthewareQ {
namespace transformations {

  using namespace qasm;

  /* \brief! Inlines gate calls
   *
   * Traverses an AST and inlines all gate calls. By default qelib calls are NOT
   * inlined, but optionally can be. Local ancillas are hoisted to the global
   * level and reused
   */
  void inline_ast(ast_context* ctx);

  /* Implementation */
  class inliner final : public replacer<inliner> {
  public:
    using replacer<inliner>::visit;

    inliner(ast_context* ctx) : ctx_(ctx), subst_(ctx) {}

    std::optional<ast_node_list> replace(decl_gate* node) override {
      // Replacement is post-order, so body should be inlined now

      if (node->has_body()) {
        auto& tmp = gate_decls_[node->identifier()];
        tmp.c_params = node->has_parameters() ? static_cast<list_ids*>(&node->parameters()) : nullptr;
        tmp.q_params = static_cast<list_ids*>(&node->arguments());
        tmp.body     = &node->body();

        return ast_node_list({});
      } else {
        // Opaque decl, don't inline
        return std::nullopt;
      }
    }

    std::optional<ast_node_list> replace(stmt_gate* node) override {
      if (gate_decls_.find(node->gate()) != gate_decls_.end()) {
        // Inline it

        // Generate a subst map
        // TODO: substitution
        ast_node_list ret;
        ret.push_back(&node->parent(), gate_decls_[node->gate()].body->copy(ctx_));
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
    std::unordered_map<std::string_view, gate_info> gate_decls_;

    // For substitutions
    substitutor subst_;
  };

  void inline_ast(ast_context* ctx) {
    auto tmp = inliner(ctx);
    tmp.visit(*ctx);
  }

}
}
