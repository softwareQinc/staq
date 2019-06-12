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

//
class expr_decl_ref final : public ast_node {

public:
	static expr_decl_ref* build(ast_context* ctx, uint32_t location, ast_node* decl)
	{
		return new (*ctx) expr_decl_ref(location, decl);
	}

	ast_node* declaration() const
	{
		return decl_;
	}

private:
	expr_decl_ref(unsigned location, ast_node* decl)
	    : ast_node(location)
	    , decl_(decl)
	{}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::expr_decl_ref;
	}

private:
	ast_node* decl_;
};

} // namespace qasm
} // namespace synthewareQ
