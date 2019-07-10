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
#include <set>

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

  /* \brief! Default override */
  static const std::set<std::string_view> default_overrides {
    "x", "y", "z", "h", "s", "sdg", "t", "tdg", "rx", "ry", "rz",
      "cz", "cy", "swap", "cx"
      };

  /* Implementation */
  class inliner final : public replacer<inliner> {
  public:
    using replacer<inliner>::visit;

    struct config {
      bool keep_declarations = true;
      std::set<std::string_view> overrides = default_overrides;
      std::string ancilla_name = "auto_anc";
    };

    inliner(ast_context* ctx) : ctx_(ctx), substitutor_(ctx) {}
    inliner(ast_context* ctx, const config& params) : ctx_(ctx), substitutor_(ctx), config_(params) {}

    std::optional<ast_node_list> replace(decl_program* node) override {
      // Replacement is post-order, so we know the max ancillas needed now

      if (max_ancilla != 0) {
        auto anc_decl = decl_register::build(ctx_, node->location(), register_type::quantum,
                                             config_.ancilla_name, max_ancilla);
        node->insert_child(node->begin(), anc_decl);
      }

      return std::nullopt;
    }


    std::optional<ast_node_list> replace(decl_gate* node) override {
      // Replacement is post-order, so body should be inlined now

      if (node->has_body()) {
        auto& tmp = gate_decls_[node->identifier()];
        tmp.c_params = node->has_parameters() ? static_cast<list_ids*>(&node->parameters()) : nullptr;
        tmp.q_params = static_cast<list_ids*>(&node->arguments());
        tmp.body     = &node->body();
        tmp.ancillas.swap(current_ancillas);
        if (num_ancilla > max_ancilla) {
          max_ancilla = num_ancilla;
        }
        num_ancilla = 0;


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

    std::optional<ast_node_list> replace(decl_ancilla* node) override {
      if (!node->is_dirty()) {
        current_ancillas.push_back(std::make_pair(node->identifier(), node->size()));
        num_ancilla += node->size();
      } else {
        std::cerr << "Error: dirty ancillas not currently supported by inliner\n";
      }
      return std::nullopt;
    }

    std::optional<ast_node_list> replace(stmt_gate* node) override {
      if (config_.overrides.find(node->gate()) != config_.overrides.end()) {
        return std::nullopt;
      }
      
      if (auto it = gate_decls_.find(node->gate()); it != gate_decls_.end()) {
        ast_node_list ret;
        ast_node* body = it->second.body->copy(ctx_);

        // Generating the substitution
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

        // For local ancillas
        auto current_offset = 0;
        for (auto& [id, num] : it->second.ancillas) {
          substs[id] = expr_reg_offset::build(ctx_, node->location(), config_.ancilla_name,
                                              expr_integer::create(ctx_, node->location(), num));
        }

        // Perform the substitution
        substitutor_.subst(substs, body);

        // Reset ancillas
        if (it->second.ancillas.size() > 0) {
          auto tmp = static_cast<list_gops*>(body);
          auto reset_builder = stmt_reset::builder(ctx_, node->location());
          reset_builder.add_child(expr_var::build(ctx_, node->location(), config_.ancilla_name));
          tmp->add_child(reset_builder.finish());
        }
        
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
      std::list<std::pair<std::string_view, uint32_t> > ancillas;
    };
    
    ast_context* ctx_;
    config config_;
    std::unordered_map<std::string_view, gate_info> gate_decls_;
    substitutor substitutor_;
    uint32_t max_ancilla = 0;

    // Gate-local accumulating values
    std::list<std::pair<std::string_view, uint32_t> > current_ancillas;
    uint32_t num_ancilla = 0;
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
