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

// A `stmt_if` node has two children.
// The children objects are in order:
namespace synthewareQ {
namespace qasm {

  class stmt_if
    : public ast_node
    , public ast_node_container<stmt_if, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) stmt_if(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      stmt_if* finish()
      {
        return statement_;
      }

    private:
      stmt_if* statement_;
    };

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_);
      for (auto& child : *this) tmp.add_child(child.copy(ctx));
      
      return tmp.finish(); 
    }

    ast_node& expression()
    {
      return *(this->begin());
    }

    ast_node& quantum_op()
    {
      auto iter = this->begin();
      return *(++iter);
    }

  private:
    stmt_if(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_if;
	}
  };

} // namespace qasm
} // namespace synthewareQ
