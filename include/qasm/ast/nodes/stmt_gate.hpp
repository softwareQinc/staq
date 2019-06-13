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

  // TODO: separate classical and quantum arguments
  class stmt_gate
    : public ast_node
    , public ast_node_container<stmt_gate, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) stmt_gate(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      stmt_gate* finish()
      {
        return statement_;
      }

    private:
      stmt_gate* statement_;
    };

    ast_node& gate()
    {
      return *(this->begin());
    }

  private:
    stmt_gate(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_gate;
	}
  };

} // namespace qasm
} // namespace synthewareQ
