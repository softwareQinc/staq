/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"

#include <list>
#include <unordered_map>
#include <set>

namespace synthewareQ {
namespace transformations {

  using namespace qasm;

  /* \brief! Applies a name substitution to an AST
   *
   * Given a partial map from identifiers to ast nodes,
   * replaces each identifier in the outer-most scope
   * with its mapping, if it exists. Used to implement
   * substitution & mapping to physical qubits
   *
   * Generally doesn't do sanity checks to ensure the
   * substituted node is in fact an access path
   */
  class substitutor final : public replacer<substitutor> {
  public:
    using replacer<substitutor>::visit;

    substitutor(ast_context* ctx) : ctx_(ctx) {}

    void subst(std::unordered_map<std::string_view, ast_node*> &substs, ast_node* node) {
      subst_ = substs;
      visit(node);
    }

    // Scoping
    void visit(decl_program* node) override {
      push_scope();
      visit_children(node);
      pop_scope();
    }
    void visit(decl_gate* node) override {
      push_scope();
      visit_children(node);
      pop_scope();
    }

    // Decls
    void visit(decl_register* node) override { add_to_scope(node->identifier()); }
    void visit(decl_param* node) override { add_to_scope(node->identifier()); }

    // Substitution
    std::optional<ast_node_list> replace(expr_var* node) override {
      auto v = node->id();
      if (free(v) && subst_.find(v) != subst_.end()) {
        auto ret = ast_node_list();
        ret.push_back(&node->parent(), subst_[v]->copy(ctx_));
        return ret;
      }

      return std::nullopt;
    }
    std::optional<ast_node_list> replace(expr_reg_offset* node) override {
      auto v = node->id();
      if (free(v) && subst_.find(v) != subst_.end()) {
        auto sub = subst_[v];
        auto ret = ast_node_list();
        // to avoid cross initialization in switch
        expr_integer* offset = nullptr;
        expr_reg_offset* deref = nullptr;
        std::string_view new_v;
        uint32_t new_index;

        switch(sub->kind()) {
        case ast_node_kinds::expr_var:
          // Replace the root variable
          new_v = (static_cast<expr_var*>(sub))->id();
          offset = static_cast<expr_integer*>(node->index().copy(ctx_));
          deref = expr_reg_offset::build(ctx_, node->location(), new_v, offset);
          ret.push_back(&node->parent(), deref);
          return ret;
        case ast_node_kinds::expr_reg_offset:
          // Replace the root variable and add the offsets
          new_v = (static_cast<expr_reg_offset*>(sub))->id();
          offset = expr_integer::create(ctx_, node->location(), node->index_numeric() +
                                        (static_cast<expr_reg_offset*>(sub))->index_numeric());
          deref = expr_reg_offset::build(ctx_, node->location(), new_v, offset);
          ret.push_back(&node->parent(), deref);
          return ret;
        default:
          std::cerr << "Error: Invalid substitution\n";
          return std::nullopt;
        }
      }
    }

  private:
    ast_context* ctx_;

    std::unordered_map<std::string_view, ast_node*> subst_; // The substitution
    std::list<std::set<std::string_view> > bound_ = { { } };            // The declared identifiers in scope

    // Scoping
    void push_scope() {
      bound_.push_front({ });
    }
    void pop_scope() {
      bound_.pop_front();
    }
    void add_to_scope(std::string_view x) {
      auto bound_vars = bound_.front();
      bound_vars.insert(x);
    }
    bool free(std::string_view x) {
      auto bound_vars = bound_.front();
      return bound_vars.find(x) == bound_vars.end();
    }

  };
    
}
}
