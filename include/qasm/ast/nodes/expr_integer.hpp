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

  class expr_integer final : public ast_node {

  public:
    static expr_integer* create(ast_context* ctx, uint32_t location, int32_t value)
    {
      return new (*ctx) expr_integer(location, value);
    }

    int32_t evaluate() const
    {
      return value_;
    }

    ast_node* copy(ast_context* ctx) const
    {
      return create(ctx, location_, value_);
    }

  private:
    expr_integer(uint32_t location, int32_t value)
      : ast_node(location)
      , value_(value)
    {}

    ast_node_kinds do_get_kind() const override
    {
      return ast_node_kinds::expr_integer;
    }

  private:
	int32_t value_;
  };

} // namespace qasm
} // namespace synthewareQ
