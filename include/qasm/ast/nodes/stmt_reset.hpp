/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../ast_context.hpp"
#include "../ast_node.hpp"
#include "../ast_node_kinds.hpp"

#include <memory>
#include <string>

namespace synthewareQ {
namespace qasm {

  // A `stmt_reset` node has one child giving the argument.
  class stmt_reset
    : public ast_node
    , public ast_node_container<stmt_reset, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) stmt_reset(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      stmt_reset* finish()
      {
        return statement_;
      }

    private:
      stmt_reset* statement_;
    };

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_);
      for (auto& child : *this) tmp.add_child(child.copy(ctx));
      
      return tmp.finish(); 
    }

    ast_node& arg()
    {
      return *(this->begin());
    }

  private:
    stmt_reset(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_reset;
	}
  };

} // namespace qasm
} // namespace synthewareQ
