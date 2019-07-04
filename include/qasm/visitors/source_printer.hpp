/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "generic/base.hpp"
#include <fmt/format.h>

#include <set>

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY = true
#endif

namespace synthewareQ {
namespace qasm {

  void print_source(ast_context*);

  /* Implementation */
  static const std::set<std::string_view> qelib_defs {
    "u3", "u2", "u1", "cx", "id", "u0", "x", "y", "z",
      "h", "s", "sdg", "t", "tdg", "rx", "ry", "rz",
      "cz", "cy", "swap", "ch", "ccx", "crz", "cu1",
      "cu3"
      };

  class source_printer final : public visitor_base<source_printer> {
  public:
    using visitor_base<source_printer>::visit;
    
    source_printer(std::ostream& os = std::cout)
      : os_(os)
    {}

    /* Declarations */
    void visit(decl_program* node)
    {
      os_ << prefix_ << "OPENQASM 2.0;" << std::endl;
      os_ << prefix_ << "include \"qelib1.inc\";" << std::endl;
      for (auto& child : *node) {
        visit(const_cast<ast_node*>(&child));
      }
    }

    void visit(decl_gate* node)
    {
      // If it's part of the standard header, don't output
      // TODO: split AST into modules and output only marked modules
      if (qelib_defs.find(node->identifier()) != qelib_defs.end()) {
        return;
      }
      
      os_ << prefix_;

      std::string gate_type;
      if (!node->has_body()) {
        // Declaration is opaque
        gate_type = "opaque";
      } else {
        // Declaration is normal
        gate_type = "gate";
      }

      // Declaration
      os_ << gate_type << " " << node->identifier();

      // Parameters (optional)
      if (node->has_parameters()) {
        os_ << "(";
        visit(const_cast<ast_node*>(&node->parameters()));
        os_ << ")";
      }

      // Arguments
      os_ << " ";
      visit(const_cast<ast_node*>(&node->arguments()));

      // Body definition (optional)
      if (node->has_body()) {
        os_ << " {" << std::endl;
        
        prefix_ += "  ";
        visit(const_cast<ast_node*>(&node->body()));
        prefix_.pop_back();
        prefix_.pop_back();

        os_ << prefix_ << "}";
      } else {
        os_ << ";";
      }
      os_ << std::endl << std::endl;
    }

    void visit(decl_oracle* node)
    {
      os_ << prefix_;

      // Declaration
      os_ << "oracle " << node->identifier();

      // Arguments
      os_ << " ";
      visit(const_cast<ast_node*>(&node->arguments()));

      // Logic file
      os_ << " { \"" << node->target() << "\" }";
      os_ << std::endl << std::endl;
    }

    void visit(decl_register* node)
    {
      os_ << prefix_;
      os_ << (node->is_quantum() ? "qreg" : "creg");
      os_ << fmt::format(" {}[{}];", node->identifier(), node->size()) << std::endl;
    }

    void visit(decl_param* node)
    {
      os_ << node->identifier();
    }

    void visit(decl_ancilla* node)
    {
      os_ << prefix_;
      os_ << (node->is_dirty() ? "dirty " : "");
      os_ << "ancilla";
      os_ << fmt::format(" {}[{}];", node->identifier(), node->size()) << std::endl;
    }

    /* Lists */
    void visit(list_gops* node)
    {
      for (auto& child : *node) {
        visit(const_cast<ast_node*>(&child));
      }
    }

    void visit(list_ids* node)
    {
      visit_list(node);
    }

    void visit(list_aps* node)
    {
      visit_list(node);
    }

    void visit(list_exprs* node)
    {
      visit_list(node);
    }

    /* Statements */
    void visit(stmt_barrier* node)
    {
      os_ << prefix_;
      os_ << "barrier ";
      visit_list(node);
      os_ << ";" << std::endl;
    }

    void visit(stmt_unitary* node)
    {
      os_ << prefix_;
      os_ << "U(";
      visit(const_cast<ast_node*>(&node->theta()));
      os_ << ",";
      visit(const_cast<ast_node*>(&node->phi()));
      os_ << ",";
      visit(const_cast<ast_node*>(&node->lambda()));
      os_ << ") ";
      visit(const_cast<ast_node*>(&node->arg()));
      os_ << ";" << std::endl;
    }

    void visit(stmt_cnot* node)
    {
      os_ << prefix_;
      os_ << "CX ";
      visit(const_cast<ast_node*>(&node->control()));
      os_ << ",";
      visit(const_cast<ast_node*>(&node->target()));
      os_ << ";" << std::endl;
    }

    void visit(stmt_gate* node)
    {
      os_ << prefix_;

      // Gate name
      os_ << node->gate();

      // Classical arguments
      if (node->has_cargs()) {
        os_ << "(";
        visit(const_cast<ast_node*>(&node->c_args()));
        os_ << ")";
      }

      // Quantum arguments
      os_ << " ";
      visit(const_cast<ast_node*>(&node->q_args()));
      os_ << ";" << std::endl;
    }

    void visit(stmt_if* node)
    {
      os_ << prefix_;
      os_ << "if (";
      visit(const_cast<ast_node*>(&node->expression()));
      os_ << ") ";
      visit(const_cast<ast_node*>(&node->quantum_op()));
      os_ << ";" << std::endl;
    }

    void visit(stmt_measure* node)
    {
      os_ << prefix_;
      os_ << "measure ";
      visit(const_cast<ast_node*>(&node->quantum_arg()));
      os_ << " -> ";
      visit(const_cast<ast_node*>(&node->classical_arg()));
      os_ << ";" << std::endl;
    }

    void visit(stmt_reset* node)
    {
      os_ << prefix_;
      os_ << "reset ";
      visit(const_cast<ast_node*>(&node->arg()));
      os_ << ";" << std::endl;
    }

    /* Expressions */
    void visit(expr_binary_op* node)
    {
      auto prev_ctx = ambiguous_;
      ambiguous_ = true;

      if (prev_ctx) {
        os_ << "(";
      }

      visit(const_cast<ast_node*>(&node->left()));

      // Print operator
      switch (node->op()) {
      case binary_ops::addition:
        os_ << "+";
        break;

      case binary_ops::subtraction:
        os_ << "-";
        break;

      case binary_ops::division:
        os_ << "/";
        break;

      case binary_ops::multiplication:
        os_ << "*";
        break;

      case binary_ops::exponentiation:
        os_ << "^";
        break;

      case binary_ops::equality:
        os_ << "==";
        break;

      default:
        std::cerr << "Error: unknown binary operator" << std::endl;
        break;
      }
      
      visit(const_cast<ast_node*>(&node->right()));

      if (prev_ctx) {
        os_ << ")";
      }

      ambiguous_ = prev_ctx;
    }

    void visit(expr_reg_offset* node)
    {
      os_ << node->id();
      os_ << "[";
      visit(const_cast<ast_node*>(&node->index()));
      os_ << "]";
    }

    void visit(expr_unary_op* node)
    {
      switch (node->op()) {
      case unary_ops::sin:
        os_ << "sin";
        break;

      case unary_ops::cos:
        os_ << "cos";
        break;

      case unary_ops::tan:
        os_ << "tan";
        break;

      case unary_ops::exp:
        os_ << "exp";
        break;

      case unary_ops::ln:
        os_ << "ln";
        break;

      case unary_ops::sqrt:
        os_ << "sqrt";
        break;

      case unary_ops::minus:
        os_ << "-";
        break;

      case unary_ops::plus:
        break;

      default:
        std::cerr << "Error: unknown unary operator" << std::endl;
        break;
      }

      auto prev_ctx = ambiguous_;
      ambiguous_ = true;
      visit(const_cast<ast_node*>(&node->subexpr()));
      ambiguous_ = prev_ctx;
    }

    void visit(expr_var* node)
    {
      os_ << node->id();
    }

    void visit(expr_integer* node)
    {
      os_ << node->evaluate();
    }

    void visit(expr_pi* node)
    {
      os_ << "pi";
    }

    void visit(expr_real* node)
    {
      os_ << node->value();
    }



  private:
    template<typename NodeT>
    void visit_list(NodeT* node)
    {
      for (auto it = node->begin(); it != node->end(); it++) {
        if (it != node->begin()) os_ << ",";
        visit(const_cast<ast_node*>(it.operator->()));
      }
    }

  private:
	std::string prefix_;
	std::ostream& os_;
    bool ambiguous_ = false;
  };

  void print_source(ast_context* ctx) {
    source_printer printer;
    printer.visit(*ctx);
  }

} // namespace qasm
} // namespace synthewareQ
