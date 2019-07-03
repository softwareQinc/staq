/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "qasm/ast/ast.hpp"

namespace synthewareQ {
namespace qasm {

  /*! \brief Base class just implementing double dispatch for derived classes. */
  template<typename Derived>
  class visitor_base {
  public:
    void visit(ast_context& context) { visit(context.root()); }
    void visit(ast_node* node) { if (node) dispatch_node(node); }
  protected:
    /* Declarations */
    virtual void visit(decl_program*) = 0;
    virtual void visit(decl_register*) = 0;
    virtual void visit(decl_param*) = 0;
    virtual void visit(decl_gate*) = 0;
    //virtual void visit(decl_opaque*) = 0;
    /* Statements */
    virtual void visit(stmt_barrier*) = 0;
    virtual void visit(stmt_cnot*) = 0;
    virtual void visit(stmt_unitary*) = 0;
    virtual void visit(stmt_gate*) = 0;
    virtual void visit(stmt_reset*) = 0;
    virtual void visit(stmt_measure*) = 0;
    virtual void visit(stmt_if*) = 0;
    /* Expressions */
    virtual void visit(expr_var*) = 0;
    virtual void visit(expr_reg_offset*) = 0;
    virtual void visit(expr_integer*) = 0;
    virtual void visit(expr_pi*) = 0;
    virtual void visit(expr_real*) = 0;
    virtual void visit(expr_binary_op*) = 0;
    virtual void visit(expr_unary_op*) = 0;
    /* Extensions */
    virtual void visit(decl_oracle*) = 0;
    virtual void visit(decl_ancilla*) = 0;
    /* Lists */
    virtual void visit(list_gops*) = 0;
    virtual void visit(list_ids*) = 0;
    virtual void visit(list_aps*) = 0;
    virtual void visit(list_exprs*) = 0;

  private:
    // Implements first leg of double dispatch
    Derived& derived()
    {
      return *static_cast<Derived*>(this);
    }

    // Implements second leg of double dispatch
    void dispatch_node(ast_node* node)
    {
      switch (node->kind()) {
        /* Containers */
      case ast_node_kinds::decl_gate:
        derived().visit(static_cast<decl_gate*>(node));
        break;

      case ast_node_kinds::decl_program:
        derived().visit(static_cast<decl_program*>(node));
        break;

      case ast_node_kinds::expr_binary_op:
        derived().visit(static_cast<expr_binary_op*>(node));
        break;

      case ast_node_kinds::expr_reg_offset:
        derived().visit(static_cast<expr_reg_offset*>(node));
        break;

      case ast_node_kinds::expr_unary_op:
        derived().visit(static_cast<expr_unary_op*>(node));
        break;

      case ast_node_kinds::list_gops:
        derived().visit(static_cast<list_gops*>(node));
        break;

      case ast_node_kinds::list_ids:
        derived().visit(static_cast<list_ids*>(node));
        break;

      case ast_node_kinds::list_aps:
        derived().visit(static_cast<list_aps*>(node));
        break;

      case ast_node_kinds::list_exprs:
        derived().visit(static_cast<list_exprs*>(node));
        break;

      case ast_node_kinds::stmt_barrier:
        derived().visit(static_cast<stmt_barrier*>(node));
        break;

      case ast_node_kinds::stmt_cnot:
        derived().visit(static_cast<stmt_cnot*>(node));
        break;

      case ast_node_kinds::stmt_gate:
        derived().visit(static_cast<stmt_gate*>(node));
        break;

      case ast_node_kinds::stmt_if:
        derived().visit(static_cast<stmt_if*>(node));
        break;

      case ast_node_kinds::stmt_measure:
        derived().visit(static_cast<stmt_measure*>(node));
        break;

      case ast_node_kinds::stmt_reset:
        derived().visit(static_cast<stmt_reset*>(node));
        break;

      case ast_node_kinds::stmt_unitary:
        derived().visit(static_cast<stmt_unitary*>(node));
        break;

        /* Leafs */
      case ast_node_kinds::decl_param:
        derived().visit(static_cast<decl_param*>(node));
        break;

      case ast_node_kinds::decl_register:
        derived().visit(static_cast<decl_register*>(node));
        break;

      case ast_node_kinds::expr_var:
        derived().visit(static_cast<expr_var*>(node));
        break;

      case ast_node_kinds::expr_integer:
        derived().visit(static_cast<expr_integer*>(node));
        break;

      case ast_node_kinds::expr_pi:
        derived().visit(static_cast<expr_pi*>(node));
        break;

      case ast_node_kinds::expr_real:
        derived().visit(static_cast<expr_real*>(node));
        break;

      case ast_node_kinds::decl_oracle:
        derived().visit(static_cast<decl_oracle*>(node));
        break;

      case ast_node_kinds::decl_ancilla:
        derived().visit(static_cast<decl_ancilla*>(node));
        break;

        return;

      }
    }
  };

} // namespace qasm
} // namespace synthewareQ
