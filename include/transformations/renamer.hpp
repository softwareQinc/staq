/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/visitor.hpp"

#include <list>
#include <unordered_map>
#include <set>

namespace synthewareQ {
namespace transformations {

  using namespace qasm;

  /* \brief! Applies a name substitution to an AST
   *
   * Given a partial map from identifiers to identifiers,
   * replaces each identifier in the outer-most scope
   * with its mapping, if it exists. Used to implement
   * substitution & mapping to physical qubits
   *
   * Modification is in-place -- i.e. without copying
   * nodes, hence if the tree is a DAG, renaming 
   * may escape the current sub-tree
   */
  void subst(std::unordered_map<std::string, std::string>, ast_node*);

  /* Implementation */
  class renamer final : public visitor<renamer> {
  public:
    using visitor<renamer>::visit;

    // Scoping
    void visit_pre(decl_program* node) override { push_scope(); }
    void visit_post(decl_program* node) override { push_scope(); }
    void visit_pre(decl_gate* node) override { push_scope(); }
    void visit_post(decl_gate* node) override { push_scope(); }

    // Decls
    void visit(decl_register* node) override { add_to_scope(node->identifier()); }
    void visit(decl_param* node) override { add_to_scope(node->identifier()); }

    // Substitution
    void visit(expr_var* node) override {
      auto v = node->id();
      if (free(v) && substs.find(v) != substs.end()) {
        node->set_id(substs[v]);
      }
    }
    void visit_pre(expr_reg_offset* node) override {
      auto v = node->id();
      if (free(v) && substs.find(v) != subst_.end()) {
        node->set_id(substs[v]);
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
