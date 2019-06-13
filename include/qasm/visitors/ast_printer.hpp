/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "generic/base.hpp"
#include <fmt/format.h>

#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY = true
#endif

namespace synthewareQ {
namespace qasm {

  class ast_printer : public visitor_base<ast_printer> {
  public:
    using visitor_base<ast_printer>::visit;
    
    ast_printer(std::ostream& os = std::cout)
      : os_(os)
    {}

    void visit(decl_program* node)
    {
      os_ << "AST for :\n";
      visit_children(node);
    }

    void visit(decl_gate* node)
    {
      os_ << fmt::format("{}|- decl_gate {}\n", prefix_, node->identifier());
      visit_children(node);
    }

    void visit(expr_binary_op* node)
    {
      os_ << fmt::format("{}|- expr_binary_op ", prefix_);
      switch (node->op()) {
      case binary_ops::addition:
        os_ << "'+'\n";
        break;

      case binary_ops::subtraction:
        os_ << "'-'\n";
        break;

      case binary_ops::division:
        os_ << "'/'\n";
        break;

      case binary_ops::multiplication:
        os_ << "'*'\n";
        break;

      case binary_ops::exponentiation:
        os_ << "'^'\n";
        break;

      case binary_ops::equality:
        os_ << "'=='\n";
        break;

      default:
        os_ << "'unknown'\n";
        break;
      }
      visit_children(node);
    }

    void visit(expr_reg_idx_ref* node)
    {
      os_ << fmt::format("{}|- expr_reg_idx_ref\n", prefix_);
      visit_children(node);
    }

    void visit(expr_unary_op* node)
    {
      os_ << fmt::format("{}|- expr_unary_op ", prefix_);
      switch (node->op()) {
      case unary_ops::sin:
        os_ << "'sin'\n";
        break;

      case unary_ops::cos:
        os_ << "'cos'\n";
        break;

      case unary_ops::tan:
        os_ << "'tan'\n";
        break;

      case unary_ops::exp:
        os_ << "'exp'\n";
        break;

      case unary_ops::ln:
        os_ << "'ln'\n";
        break;

      case unary_ops::sqrt:
        os_ << "'sqrt'\n";
        break;

      case unary_ops::minus:
        os_ << "'minus'\n";
        break;

      case unary_ops::plus:
        os_ << "'plus'\n";
        break;

      default:
        os_ << "'unknown'\n";
        break;
      }
      visit_children(node);
    }

    void visit(list_gops* node)
    {
      os_ << fmt::format("{}|- list_gops ({})\n", prefix_, node->num_children());
      visit_children(node);
    }

    void visit(list_ids* node)
    {
      os_ << fmt::format("{}|- list_ids ({})\n", prefix_, node->num_children());
      visit_children(node);
    }

    void visit(stmt_barrier* node)
    {
      os_ << fmt::format("{}|- stmt_barrier\n", prefix_);
      visit_children(node);
    }

    void visit(stmt_cnot* node)
    {
      os_ << fmt::format("{}|- stmt_cnot\n", prefix_);
      visit_children(node);
    }

    void visit(stmt_gate* node)
    {
      os_ << fmt::format("{}|- stmt_gate\n", prefix_);
      visit_children(node);
    }

    void visit(stmt_if* node)
    {
      os_ << fmt::format("{}|- stmt_if\n", prefix_);
      visit_children(node);
    }

    void visit(stmt_measure* node)
    {
      os_ << fmt::format("{}|- stmt_measure\n", prefix_);
      visit_children(node);
    }

    void visit(stmt_reset* node)
    {
      os_ << fmt::format("{}|- stmt_reset\n", prefix_);
      visit_children(node);
    }

    void visit(stmt_unitary* node)
    {
      os_ << fmt::format("{}|- stmt_unitary\n", prefix_);
      visit_children(node);
    }

    /* Leafs */
    void visit(decl_param* node)
    {
      os_ << fmt::format("{}|- decl_param {}\n", prefix_, node->identifier());
    }

    void visit(decl_register* node)
    {
      os_ << fmt::format("{}|- decl_register {} ({}:{})\n", prefix_, node->identifier(),
                         node->is_quantum() ? "Quantum" : "Classical",
                         node->size());
    }

    void visit(expr_decl_ref* node)
    {
      (void) node;
      os_ << fmt::format("{}|- expr_decl_ref\n", prefix_);
    }

    void visit(expr_integer* node)
    {
      os_ << fmt::format("{}|- expr_integer {}\n", prefix_, node->evaluate());
    }

    void visit(expr_pi* node)
    {
      (void) node;
      os_ << fmt::format("{}|- expr_pi\n", prefix_);
    }

    void visit(expr_real* node)
    {
      os_ << fmt::format("{}|- expr_real {}\n", prefix_, node->value());
	}

    void visit(decl_oracle* node)
    {
      os_ << fmt::format("{}|- decl_oracle {}\n", prefix_, node->identifier());
      visit_children(node);
    }

	void visit(decl_ancilla* node)
	{
      os_ << fmt::format("{}|- decl_ancilla {} ({}:{})\n", prefix_, node->identifier(),
                         node->is_dirty() ? "Dirty" : "Clean",
                         node->size());
	}

private:
	template<typename NodeT>
	void visit_children(NodeT* node)
	{
		prefix_ += "| ";
		for (ast_node& child : *node) {
			visit(&child);
		}
		prefix_.pop_back();
		prefix_.pop_back();
	}

private:
	std::string prefix_;
	std::ostream& os_;
};

}
}
