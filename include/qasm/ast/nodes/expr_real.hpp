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

class expr_real final : public ast_node {
public:
	static expr_real* create(ast_context* ctx, uint32_t location, double value)
	{
		return new (*ctx) expr_real(location, value);
	}

	double evaluate() const
	{
		return value_;
	}

	double value() const
	{
		return value_;
	}

private:
	expr_real(uint32_t location, double value)
	    : ast_node(location)
	    , value_(value)
	{}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::expr_real;
	}

private:
	double value_;
};

} // namespace qasm
} // namespace synthewareQ
