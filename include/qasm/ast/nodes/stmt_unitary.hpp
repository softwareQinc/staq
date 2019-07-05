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

  // A `stmt_unitary` node has four children.
  // The children objects are in order:
  class stmt_unitary
    : public ast_node
    , public ast_node_container<stmt_unitary, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) stmt_unitary(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      stmt_unitary* finish()
      {
        return statement_;
      }

    private:
      stmt_unitary* statement_;
    };

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_);
      for (auto& child : *this) tmp.add_child(child.copy(ctx));
      
      return tmp.finish(); 
    }

    ast_node& theta()
    {
      return *(this->begin());
    }

    ast_node& phi()
    {
      auto iter = this->begin();
      return *(++iter);
    }

    ast_node& lambda()
    {
      auto iter = this->begin();
      ++iter;
      return *(++iter);
    }
  
    ast_node& arg()
    {
      auto iter = this->begin();
      ++iter;
      ++iter;
      return *(++iter);
    }

  private:
    stmt_unitary(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_unitary;
	}
  };

} // namespace qasm
} // namespace synthewareQ
