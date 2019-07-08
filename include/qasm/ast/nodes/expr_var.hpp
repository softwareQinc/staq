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

  class expr_var final : public ast_node {

  public:
    static expr_var* build(ast_context* ctx, uint32_t location, std::string_view id)
    {
      return new (*ctx) expr_var(location, id);
    }

    ast_node* copy(ast_context* ctx) const
    {
      return build(ctx, location_, id_);
    }

    std::string_view id() const
    {
      return id_;
    }

    void set_id(std::string const x)
    {
      id_ = x;
    }

  private:
    expr_var(uint32_t location, std::string_view id)
      : ast_node(location)
      , id_(id)
    {}

    ast_node_kinds do_get_kind() const override
    {
      return ast_node_kinds::expr_var;
    }

  private:
    std::string id_;
  };

} // namespace qasm
} // namespace synthewareQ
