/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "base.hpp"

#include <unordered_map>

namespace synthewareQ {
namespace qasm {

  /*! \brief Generic node replacement visitor
  *
  * Usage: override the replace methods for the nodes desired.
  * Returning the original node pointer leaves the node unchanged,
  * returning null deletes the node and any other pointer replaces
  * the current node
  */
  template <typename Derived>
  class replacer : public visitor_base<Derived> {
  public:
    using visitor_base<Derived>::visit;
    
  protected:
    /* Declarations */
    virtual ast_node* replace(decl_program* node) { return node; };
    virtual ast_node* replace(decl_register* node) { return node; };
    virtual ast_node* replace(decl_param* node) { return node; };
    virtual ast_node* replace(decl_gate* node) { return node; };
    //virtual ast_node* replace(decl_opaque* node) { return node; };
    /* Statements */
    virtual ast_node* replace(stmt_barrier* node) { return node; };
    virtual ast_node* replace(stmt_cnot* node) { return node; };
    virtual ast_node* replace(stmt_unitary* node) { return node; };
    virtual ast_node* replace(stmt_gate* node) { return node; };
    virtual ast_node* replace(stmt_reset* node) { return node; };
    virtual ast_node* replace(stmt_measure* node) { return node; };
    virtual ast_node* replace(stmt_if* node) { return node; };
    /* Expressions */
    virtual ast_node* replace(expr_var* node) { return node; };
    virtual ast_node* replace(expr_reg_offset* node) { return node; };
    virtual ast_node* replace(expr_integer* node) { return node; };
    virtual ast_node* replace(expr_pi* node) { return node; };
    virtual ast_node* replace(expr_real* node) { return node; };
    virtual ast_node* replace(expr_binary_op* node) { return node; };
    virtual ast_node* replace(expr_unary_op* node) { return node; };
    /* Extensions */
    virtual ast_node* replace(decl_oracle* node) { return node; };
    virtual ast_node* replace(decl_ancilla* node) { return node; };
    /* Lists */
    virtual ast_node* replace(list_gops* node) { return node; };
    virtual ast_node* replace(list_ids* node) { return node; };
    virtual ast_node* replace(list_aps* node) { return node; };
    virtual ast_node* replace(list_exprs* node) { return node; };

    void visit(decl_program* node)
    {
      visit_children(node);
      replacement_ = replace(node);

    }
    void visit(decl_register* node) { replacement_ = replace(node); }
    void visit(decl_param* node) { replacement_ = replace(node); }
    void visit(decl_gate* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }


    void visit(stmt_barrier* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(stmt_cnot* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(stmt_unitary* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(stmt_gate* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(stmt_reset* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(stmt_measure* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(stmt_if* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }

    void visit(expr_var* node) { replacement_ = replace(node); }
    void visit(expr_reg_offset* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(expr_integer* node) { replacement_ = replace(node); }
    void visit(expr_pi* node) { replacement_ = replace(node); }
    void visit(expr_real* node) { replacement_ = replace(node); }
    void visit(expr_binary_op* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(expr_unary_op* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }

    void visit(decl_oracle* node) {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(decl_ancilla* node) { replacement_ = replace(node); }

    void visit(list_gops* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(list_ids* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(list_aps* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }
    void visit(list_exprs* node)
    {
      visit_children(node);
      replacement_ = replace(node);
    }

  private:
    ast_node* replacement_;

	template<typename NodeT>
	void visit_children(NodeT* node)
	{
      for (auto it = node->begin(); it != node->end(); it++) {
        visit(it.operator->());
        if (replacement_ == nullptr) {
          it = node->delete_child(it);
        } else if (replacement_ != it.operator->()) {
          it = node->set_child(it, replacement_);
		}
      }
	}
    
  };


  /*! \brief Utility for bulk node replacement
  *
  * Given a hash map consisting of nodes to be replaced with their
  * replacement, perform all replacements
  */
  class bulk_replacer final : public visitor_base<bulk_replacer> {
  public:
    using visitor_base<bulk_replacer>::visit;
    friend visitor_base<bulk_replacer>;
    friend void bulk_replace(ast_context&, std::unordered_map<ast_node*, ast_node*>);

    bulk_replacer(std::unordered_map<ast_node*, ast_node*> replacements)
      : replacements_(replacements)
    {}

  protected:
    void visit(decl_program* node) { visit_children(node); }
    void visit(decl_register* node) { }
    void visit(decl_param* node) { }
    void visit(decl_gate* node) { visit_children(node); }
    void visit(stmt_barrier* node) { visit_children(node); }
    void visit(stmt_cnot* node) { visit_children(node); }
    void visit(stmt_unitary* node) { visit_children(node); }
    void visit(stmt_gate* node) { visit_children(node); }
    void visit(stmt_reset* node) { visit_children(node); }
    void visit(stmt_measure* node) { visit_children(node); }
    void visit(stmt_if* node) { visit_children(node); }
    void visit(expr_var* node) { }
    void visit(expr_reg_offset* node) { visit_children(node); }
    void visit(expr_integer* node) { }
    void visit(expr_pi* node) { }
    void visit(expr_real* node) { }
    void visit(expr_binary_op* node) { visit_children(node); }
    void visit(expr_unary_op* node) { visit_children(node); }
    void visit(decl_oracle* node) { visit_children(node); }
    void visit(decl_ancilla* node) { }
    void visit(list_gops* node) { visit_children(node); }
    void visit(list_ids* node) { visit_children(node); }
    void visit(list_aps* node) { visit_children(node); }
    void visit(list_exprs* node) { visit_children(node); }

  private:
    std::unordered_map<ast_node*, ast_node*> replacements_;

	template<typename NodeT>
	void visit_children(NodeT* node)
	{
      for (auto it = node->begin(); it != node->end(); it) {
        auto child = &(*it);

        visit(child);
        if (replacements_.find(child) != replacements_.end()) {
          if (replacements_[child] == nullptr) {
            it = node->delete_child(it);
          } else {
            it = node->set_child(it, replacements_[child]);
            it++;
          }
        } else {
          it++;
        }
      }
	}
    
  };

  void bulk_replace(ast_context& ctx, std::unordered_map<ast_node*, ast_node*> replacements) {
    auto replacer = bulk_replacer(replacements);
    replacer.visit(ctx);
  }

}
}
