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
  void subst(std::unordered_map<std::string, std::string>, ast_node*);
  // TODO: this is another instance where a proper class hierarchy would help

  /* Implementation */
  class substitor final : public replacer<substitutor> {
  public:
    using visitor<renamer>::visit;

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
      if (free(v) && substs.find(v) != substs.end()) {
        return substs[v]->copy(ctx_);
      }
    }
    std::optional<ast_node_list> replace(expr_reg_offset* node) override {
      auto v = node->id();
      if (free(v) && substs.find(v) != subst_.end()) {
        auto subst = substs[v];
        expr_integer* offset = nullptr; // to avoid cross initialization in switch
        switch(subst->kind()) {
        case ast_nodes_kinds::expr_var:
          // Replace the root variable
          return expr_reg_offset::build(ctx_, node->location(), subst->id(), node->offset.copy(ctx_));
        case ast_node_kinds::expr_reg_offset:
          // Replace the root variable and add the offsets
          offset = expr_integer::create(ctx_, node->location(), node->index_numeric() +
                                        (static_cast<expr_reg_offset*>(subst))->index_numeric);
          return expr_reg_offset::build(ctx_, node->location(), subst->id(), offset);
        default:
          std::cerr << "Error: Invalid substitution\n";
          return std::nullopt;
        }
      }
    }

  private:
    std::unordered_map<std::string, std::string> subst_; // The substitution
    std::list<std::set<std::string> > bound_;            // The declared identifiers in scope

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
    
  void subst(std::unordered_map<std::string, std::string> substs, ast_node* node);
    auto tmp = renamer(substs);
    tmp.visit(node);
  }

}
}
