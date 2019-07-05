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

  // A `stmt_measure` node has two children.
  // The children objects are in order:
  class stmt_measure
    : public ast_node
    , public ast_node_container<stmt_measure, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) stmt_measure(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      stmt_measure* finish()
      {
        return statement_;
      }

    private:
      stmt_measure* statement_;
    };

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_);
      for (auto& child : *this) tmp.add_child(child.copy(ctx));
      
      return tmp.finish(); 
    }

    ast_node& quantum_arg()
    {
      return *(this->begin());
    }

    ast_node& classical_arg()
    {
      auto iter = this->begin();
      return *(++iter);
    }

  private:
    stmt_measure(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_measure;
	}
  };

} // namespace qasm
} // namespace synthewareQ
