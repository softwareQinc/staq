/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "base.hpp"

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
    virtual ast_node* replace(expr_decl_ref* node) { return node; };
    virtual ast_node* replace(expr_reg_idx_ref* node) { return node; };
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

    void visit(expr_decl_ref* node) { replacement_ = replace(node); }
    void visit(expr_reg_idx_ref* node)
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

}
}
