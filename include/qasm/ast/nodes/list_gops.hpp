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

class list_gops
    : public ast_node
    , public ast_node_container<list_gops, ast_node> {
public:
	class builder {
	public:
		explicit builder(ast_context* ctx, uint32_t location)
		    : node_(new (*ctx) list_gops(location))
		{}

		void add_child(ast_node* child)
		{
			node_->add_child(child);
		}

		list_gops& get()
		{
			return *node_;
		}

		list_gops* finish()
		{
			return node_;
		}

	private:
		list_gops* node_;
	};

private:
	list_gops(uint32_t location)
	    : ast_node(location)
	{}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::list_gops;
	}
};

} // namespace qasm
} // namespace synthewareQ
