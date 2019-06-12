/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "ast_context.hpp"
#include "ast_node.hpp"
#include "ast_node_kinds.hpp"
#include "visitor.hpp"

#include <set>

namespace synthewareQ {
namespace qasm {

static const std::set<std::string_view> qelib_defs {
  "u3", "u2", "u1", "cx", "id", "u0", "x", "y", "z",
  "h", "s", "sdg", "t", "tdg", "rx", "ry", "rz",
  "cz", "cy", "swap", "ch", "ccx", "crz", "cu1",
  "cu3"
};

class pretty_printer : public visitor_base<pretty_printer> {
public:
	pretty_printer(std::ostream& os = std::cout)
	: os_(os)
	{}

	/* Declarations */
	void visit_decl_program(decl_program* node)
	{
      os_ << prefix_ << "OPENQASM 2.0;" << std::endl;
      os_ << prefix_ << "include \"qelib1.inc\";" << std::endl;
      for (auto& child : *node) {
        visit(const_cast<ast_node*>(&child));
      }
	}

	void visit_decl_gate(decl_gate* node)
    {
      // If it's part of the standard header, don't output
      // TODO: a more sensible solution would be to have
      // the included modules as part of the AST, then only
      // output declarations not declared in an included module.
      if (qelib_defs.find(node->identifier()) != qelib_defs.end()) {
        return;
      }
      
      os_ << prefix_;

      std::string gate_type;
      if (!(node->is_classical() || node->has_body())) {
        // Declaration is opaque
        gate_type = "opaque";
      } else if (node->is_classical() && !(node->has_body())) {
        // Declaration is an oracle
        gate_type = "oracle";
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
      } else if (node->is_classical()) {
        os_ << " { \"";

        visit(const_cast<ast_node*>(&node->file()));

        os_ << "\" }";
      } else {
        os_ << ";";
      }
      os_ << std::endl << std::endl;
	}

	void visit_decl_register(decl_register* node)
	{
      os_ << prefix_;
      os_ << (node->is_quantum() ? "qreg" : "creg");
      os_ << fmt::format(" {}[{}];", node->identifier(), node->size()) << std::endl;
	}

	void visit_decl_param(decl_param* node)
	{
		os_ << node->identifier();
	}

	void visit_decl_ancilla(decl_ancilla* node)
	{
      os_ << prefix_;
      os_ << (node->is_dirty() ? "dirty " : "");
      os_ << "ancilla";
      os_ << fmt::format(" {}[{}];", node->identifier(), node->size()) << std::endl;
	}

    /* Lists */
	void visit_list_gops(list_gops* node)
	{
      for (auto& child : *node) {
        visit(const_cast<ast_node*>(&child));
      }
	}

	void visit_list_ids(list_ids* node)
	{
      visit_children(node);
	}

    /* Statements */
	void visit_stmt_barrier(stmt_barrier* node)
	{
      os_ << prefix_;
      os_ << "barrier ";
      visit_children(node);
      os_ << ";" << std::endl;
	}

	void visit_stmt_unitary(stmt_unitary* node)
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

	void visit_stmt_cnot(stmt_cnot* node)
	{
      os_ << prefix_;
      os_ << "CX ";
      visit(const_cast<ast_node*>(&node->control()));
      os_ << ",";
      visit(const_cast<ast_node*>(&node->target()));
      os_ << ";" << std::endl;
	}

	void visit_stmt_gate(stmt_gate* node)
    { // Again very nasty
      os_ << prefix_;

      auto decl_ref = static_cast<expr_decl_ref*>(node->begin().operator->());
      auto decl = static_cast<decl_gate*>(decl_ref->declaration());
      visit(const_cast<ast_node*>(node->begin().operator->()));

      // Parameters
      auto it = node->begin();
      it++;

      if (decl->has_parameters()) {
        auto params = static_cast<list_ids*>(&decl->parameters());
        os_ << "(";
        for (auto i = 0; i < params->num_children(); i++) {
          if (i != 0) os_ << ",";
          visit(const_cast<ast_node*>(it++.operator->()));
        }
        os_ << ")";
      }

      // Arguments
      os_ << " ";
      for (auto i = 0; it != node->end(); it++, i++) {
        if (i != 0) os_ << ",";
        visit(const_cast<ast_node*>(it.operator->()));
      }
      os_ << ";" << std::endl;
	}

	void visit_stmt_if(stmt_if* node)
	{
      os_ << prefix_;
      os_ << "if (";
      visit(const_cast<ast_node*>(&node->expression()));
      os_ << ") ";
      visit(const_cast<ast_node*>(&node->quantum_op()));
      os_ << ";" << std::endl;
	}

	void visit_stmt_measure(stmt_measure* node)
	{
      os_ << prefix_;
      os_ << "measure ";
      visit(const_cast<ast_node*>(&node->quantum_arg()));
      os_ << " -> ";
      visit(const_cast<ast_node*>(&node->classical_arg()));
      os_ << ";" << std::endl;
	}

	void visit_stmt_reset(stmt_reset* node)
	{
      os_ << prefix_;
      os_ << "reset ";
      visit(const_cast<ast_node*>(&node->arg()));
      os_ << ";" << std::endl;
	}

    /* Expressions */
	void visit_expr_binary_op(expr_binary_op* node)
	{
      if (node->left().has_children()) {
        os_ << "(";
        visit(const_cast<ast_node*>(&node->left()));
        os_ << ")";
      } else {
        visit(const_cast<ast_node*>(&node->left()));
      }

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
      
      if (node->right().has_children()) {
        os_ << "(";
        visit(const_cast<ast_node*>(&node->right()));
        os_ << ")";
      } else {
        visit(const_cast<ast_node*>(&node->left()));
      }
	}

	void visit_expr_reg_idx_ref(expr_reg_idx_ref* node)
	{
      auto it = node->begin();
      visit(const_cast<ast_node*>(it.operator->()));
      os_ << "[";
      visit(const_cast<ast_node*>((++it).operator->()));
      os_ << "]";
	}

	void visit_expr_unary_op(expr_unary_op* node)
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

      if (node->begin()->has_children()) {
        os_ << "(";
        visit(const_cast<ast_node*>(node->begin().operator->()));
        os_ << ")";
      } else {
        visit(const_cast<ast_node*>(node->begin().operator->()));
      }
	}

	void visit_expr_decl_ref(expr_decl_ref* node)
	{
      auto decl = node->declaration();
      switch (decl->kind()) {
      case ast_node_kinds::decl_register:
        os_ << static_cast<decl_register*>(decl)->identifier();
        break;
      case ast_node_kinds::decl_param:
        os_ << static_cast<decl_param*>(decl)->identifier();
        break;
      case ast_node_kinds::decl_gate:
        os_ << static_cast<decl_gate*>(decl)->identifier();
        break;
      case ast_node_kinds::decl_ancilla:
        os_ << static_cast<decl_ancilla*>(decl)->identifier();
        break;
      default:
        std::cerr << "Error: could not find declared identifier" << std::endl;
        break;
      }
	}

	void visit_expr_integer(expr_integer* node)
	{
		os_ << node->evaluate();
	}

	void visit_expr_pi(expr_pi* node)
	{
		os_ << "pi";
	}

	void visit_expr_real(expr_real* node)
	{
		os_ << node->value();
	}

    /* Misc */
    void visit_logic_file(logic_file* node)
    {
        os_ << node->filename();
    }


private:
	template<typename NodeT>
	void visit_children(NodeT* node)
	{
      for (auto it = node->begin(); it != node->end(); it++) {
        if (it != node->begin()) os_ << ",";
        visit(const_cast<ast_node*>(it.operator->()));
      }
	}

private:
	std::string prefix_;
	std::ostream& os_;
};

} // namespace qasm
} // namespace synthewareQ
