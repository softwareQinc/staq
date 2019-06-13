/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../ast_context.hpp"
#include "../ast_node.hpp"
#include "../ast_node_kinds.hpp"

#include <memory>
#include <string>

namespace synthewareQ {
namespace qasm {

  class expr_reg_idx_ref
    : public ast_node
    , public ast_node_container<expr_reg_idx_ref, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) expr_reg_idx_ref(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      expr_reg_idx_ref* finish()
      {
        return statement_;
      }

    private:
      expr_reg_idx_ref* statement_;
    };

    ast_node& var()
    {
      return *(this->begin());
    }

    ast_node& index()
    {
      return *(++(this->begin()));
    }


  private:
    expr_reg_idx_ref(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::expr_reg_idx_ref;
	}
  };

} // namespace qasm
} // namespace synthewareQ
