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

class stmt_barrier
    : public ast_node
    , public ast_node_container<stmt_barrier, ast_node> {
public:
	class builder {
	public:
		explicit builder(ast_context* ctx, uint32_t location)
		    : statement_(new (*ctx) stmt_barrier(location))
		{}

		void add_child(ast_node* child)
		{
			statement_->add_child(child);
		}

		stmt_barrier& get()
		{
			return *statement_;
		}

		stmt_barrier* finish()
		{
			return statement_;
		}

	private:
		stmt_barrier* statement_;
	};

private:
	stmt_barrier(uint32_t location)
	    : ast_node(location)
	{}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::stmt_barrier;
	}
};

} // namespace qasm
} // namespace synthewareQ
