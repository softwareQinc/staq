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
#include <ostream>
#include <string>

namespace synthewareQ {
namespace qasm {

  enum class unary_ops {
    unknown = 0,
    sin = 1,
    cos = 2,
    tan = 4,
    exp = 8,
    ln = 16,
    sqrt = 32,
    minus = 64,
    plus = 128,
  };

  //
  class expr_unary_op
    : public ast_node
    , public ast_node_container<expr_unary_op, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location, unary_ops op)
        : expression_(new (*ctx) expr_unary_op(location, op))
      {}

      void add_child(ast_node* child)
      {
        expression_->add_child(child);
      }

      expr_unary_op* finish()
      {
        return expression_;
      }

    private:
      expr_unary_op* expression_;
    };

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_, op());
      tmp.add_child(begin()->copy(ctx));

      return tmp.finish();
    }

    unary_ops op() const
    {
      return static_cast<unary_ops>(this->config_bits_);
    }

    bool is(unary_ops op) const
    {
      return this->config_bits_ == static_cast<uint32_t>(op);
    }

    ast_node& subexpr()
    {
      return *(this->begin());
    }

  private:
    expr_unary_op(uint32_t location, unary_ops op)
      : ast_node(location)
    {
      this->config_bits_ = static_cast<uint32_t>(op);
	}

	ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::expr_unary_op;
	}
  };

} // namespace qasm
} // namespace synthewareQ
