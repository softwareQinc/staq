/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "base.hpp"

namespace synthewareQ {
namespace qasm {

  /*! \brief Generic complete traversal 
  *
  * Usage: override the visit_pre and visit_post functions only for the nodes desired.
  * The internal logic will visit all nodes in the tree, applying the
  * overloaded pre and post visitors before and after visiting all children 
  */
  template <typename Derived>
  class visitor : public visitor_base<Derived> {
  public:
    using visitor_base<Derived>::visit;
    
  protected:

    // Nodes
    virtual void visit_pre(decl_program*) {}
    virtual void visit_post(decl_program*) {}

    virtual void visit_pre(decl_gate*) {}
    virtual void visit_post(decl_gate*) {}

    virtual void visit_pre(stmt_barrier*) {}
    virtual void visit_post(stmt_barrier*) {}

    virtual void visit_pre(stmt_cnot*) {}
    virtual void visit_post(stmt_cnot*) {}

    virtual void visit_pre(stmt_unitary*) {}
    virtual void visit_post(stmt_unitary*) {}

    virtual void visit_pre(stmt_gate*) {}
    virtual void visit_post(stmt_gate*) {}

    virtual void visit_pre(stmt_reset*) {}
    virtual void visit_post(stmt_reset*) {}

    virtual void visit_pre(stmt_measure*) {}
    virtual void visit_post(stmt_measure*) {}

    virtual void visit_pre(stmt_if*) {}
    virtual void visit_post(stmt_if*) {}

    virtual void visit_pre(expr_reg_offset*) {}
    virtual void visit_post(expr_reg_offset*) {}

    virtual void visit_pre(expr_binary_op*) {}
    virtual void visit_post(expr_binary_op*) {}

    virtual void visit_pre(expr_unary_op*) {}
    virtual void visit_post(expr_unary_op*) {}

    virtual void visit_pre(decl_oracle*) {}
    virtual void visit_post(decl_oracle*) {}

    virtual void visit_pre(list_gops*) {}
    virtual void visit_post(list_gops*) {}

    virtual void visit_pre(list_ids*) {}
    virtual void visit_post(list_ids*) {}

    virtual void visit_pre(list_aps*) {}
    virtual void visit_post(list_aps*) {}

    virtual void visit_pre(list_exprs*) {}
    virtual void visit_post(list_exprs*) {}

    // Leaves
    virtual void visit(decl_register*) {}
    virtual void visit(decl_param*) {}
    virtual void visit(expr_var*) {}
    virtual void visit(expr_integer*) {}
    virtual void visit(expr_pi*) {}
    virtual void visit(expr_real*) {}
    virtual void visit(decl_ancilla*) {}

  private:
    void visit(decl_program* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(decl_gate* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_barrier* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_cnot* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_unitary* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_gate* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_reset* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_measure* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(stmt_if* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(expr_reg_idx_ref* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(expr_binary_op* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(expr_unary_op* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(decl_oracle* node) {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(list_gops* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(list_ids* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }

    void visit(list_aps* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
    void visit(list_exprs* node)
    {
      visit_pre(node);
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      visit_post(node);
    }
  };

}
}
