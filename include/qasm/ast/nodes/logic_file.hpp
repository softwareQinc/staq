/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../ast_context.hpp"
#include "../ast_node.hpp"
#include "../ast_node_kinds.hpp"

#include <memory>
#include <string>

namespace synthewareQ {
namespace qasm {

class logic_file final : public ast_node {
public:
	static logic_file* build(ast_context* ctx, uint32_t location, std::string_view fname)
	{
		auto result = new (*ctx) logic_file(location, fname);
		return result;
	}

	std::string_view filename() const
	{
		return fname_;
	}

private:
	logic_file(uint32_t location, std::string_view fname)
	    : ast_node(location)
	    , fname_(fname)
	{}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::logic_file;
	}

private:
    std::string fname_;
};

} // namespace qasm
} // namespace synthewareQ
