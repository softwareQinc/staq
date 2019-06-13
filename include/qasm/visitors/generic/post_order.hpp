/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "base.hpp"

namespace synthewareQ {
namespace qasm {

  /*! \brief Generic post-order traversal 
  *
  * Usage: override the visit_post functions only for the nodes desired.
  * The internal logic will visit all nodes in the tree, applying the
  * overloaded post visitors after visiting all children 
  */
  template <typename Derived>
  class post_visitor : public visitor_base<Derived> {
  public:
    using visitor_base<Derived>::visit;
    
  protected:
    /* Declarations */
    virtual void visit_post(decl_program*) {}
    virtual void visit_post(decl_register*) {}
    virtual void visit_post(decl_param*) {}
    virtual void visit_post(decl_gate*) {}
    //virtual void visit_post(decl_opaque*) {}
    /* Statements */
    virtual void visit_post(stmt_barrier*) {}
    virtual void visit_post(stmt_cnot*) {}
    virtual void visit_post(stmt_unitary*) {}
    virtual void visit_post(stmt_gate*) {}
    virtual void visit_post(stmt_reset*) {}
    virtual void visit_post(stmt_measure*) {}
    virtual void visit_post(stmt_if*) {}
    /* Expressions */
    virtual void visit_post(expr_decl_ref*) {}
    virtual void visit_post(expr_reg_idx_ref*) {}
    virtual void visit_post(expr_integer*) {}
    virtual void visit_post(expr_pi*) {}
    virtual void visit_post(expr_real*) {}
    virtual void visit_post(expr_binary_op*) {}
    virtual void visit_post(expr_unary_op*) {}
    /* Extensions */
    virtual void visit_post(decl_oracle*) {}
    virtual void visit_post(decl_ancilla*) {}
    /* Lists */
    virtual void visit_post(list_gops*) {}
    virtual void visit_post(list_ids*) {}

  private:
    void visit(decl_program* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);

    }
    void visit(decl_register* node) { visit_post(node); }
    void visit(decl_param* node) { visit_post(node); }
    void visit(decl_gate* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }


    void visit(stmt_barrier* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_cnot* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_unitary* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_gate* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_reset* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_measure* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_if* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }

    void visit(expr_decl_ref* node) { visit_post(node); }
    void visit(expr_reg_idx_ref* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(expr_integer* node) { visit_post(node); }
    void visit(expr_pi* node) { visit_post(node); }
    void visit(expr_real* node) { visit_post(node); }
    void visit(expr_binary_op* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(expr_unary_op* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }

    void visit(decl_oracle* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(decl_ancilla* node) { visit_post(node); }

    void visit(list_gops* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(list_ids* node)
    {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
  };

}
}
