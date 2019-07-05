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
  * Returning nullopt leaves the node unchanged, returning 
  * a list of nodes (via intrusive list) deletes the node and replaces
  * it with the given list spliced in
  */
  template <typename Derived>
  class replacer : public visitor_base<Derived> {
  public:
    using visitor_base<Derived>::visit;
    
  protected:
    std::optional<ast_node_list> replacement_;

    /* Declarations */
    virtual std::optional<ast_node_list> replace(decl_program* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(decl_register* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(decl_param* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(decl_gate* node) { return std::nullopt; };
    //virtual std::optional<ast_node_list> replace(decl_opaque* node) { return std::nullopt; };
    /* Statements */
    virtual std::optional<ast_node_list> replace(stmt_barrier* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(stmt_cnot* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(stmt_unitary* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(stmt_gate* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(stmt_reset* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(stmt_measure* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(stmt_if* node) { return std::nullopt; };
    /* Expressions */
    virtual std::optional<ast_node_list> replace(expr_var* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(expr_reg_offset* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(expr_integer* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(expr_pi* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(expr_real* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(expr_binary_op* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(expr_unary_op* node) { return std::nullopt; };
    /* Extensions */
    virtual std::optional<ast_node_list> replace(decl_oracle* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(decl_ancilla* node) { return std::nullopt; };
    /* Lists */
    virtual std::optional<ast_node_list> replace(list_gops* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(list_ids* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(list_aps* node) { return std::nullopt; };
    virtual std::optional<ast_node_list> replace(list_exprs* node) { return std::nullopt; };

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

	template<typename NodeT>
	void visit_children(NodeT* node)
	{
      for (auto it = node->begin(); it != node->end(); it) {
        visit(it.operator->());
        if (replacement_.has_value()) {
          node->insert_children(it, *replacement_);
          it = node->delete_child(it);
        } else {
          it++;
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
    friend void bulk_replace(ast_context&, std::unordered_map<ast_node*, ast_node_list>);

    bulk_replacer(std::unordered_map<ast_node*, ast_node_list> replacements)
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
    std::unordered_map<ast_node*, ast_node_list> replacements_;

	template<typename NodeT>
	void visit_children(NodeT* node)
	{
      for (auto it = node->begin(); it != node->end(); it) {
        auto child = &(*it);

        visit(child);
        if (replacements_.find(child) != replacements_.end()) {
          node->insert_children(it, replacements_[child]);
          it = node->delete_child(it);
        } else {
          it++;
        }
      }
	}
    
  };

  void bulk_replace(ast_context& ctx, std::unordered_map<ast_node*, ast_node_list> replacements) {
    auto replacer = bulk_replacer(replacements);
    replacer.visit(ctx);
  }

}
}
