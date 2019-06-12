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

enum class binary_ops {
	unknown = 0,
	// Arithmetic
	addition = 1,
	subtraction = 2,
	division = 4,
	multiplication = 8,
	exponentiation = 16,
	// Relational
	equality = 32,
};

// A `decl_gate` node has two children, one of which optional.
// The children objects are in order:
//
// * A `list_ids *` for the parameter list.
//    Always present.
//
// * A "list_ids *" for the qubit argument list. (At least one qubit argument is required)
//    Always present.
class expr_binary_op
    : public ast_node
    , public ast_node_container<expr_binary_op, ast_node> {
public:
	class builder {
	public:
		explicit builder(ast_context* ctx, uint32_t location, binary_ops op)
		    : expression_(new (*ctx) expr_binary_op(location, op))
		{}

		void add_child(ast_node* child)
		{
			expression_->add_child(child);
		}

		expr_binary_op* finish()
		{
			return expression_;
		}

	private:
		expr_binary_op* expression_;
	};

	binary_ops op() const
	{
		return static_cast<binary_ops>(this->config_bits_);
	}

	bool is(binary_ops op) const
	{
		return this->config_bits_ == static_cast<uint32_t>(op);
	}

	ast_node& left()
	{
		return *(this->begin());
	}

	ast_node& right()
	{
		auto iter = this->begin();
		return *(++iter);
	}

private:
	expr_binary_op(uint32_t location, binary_ops op)
	    : ast_node(location)
	{
		this->config_bits_ = static_cast<uint32_t>(op);
	}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::expr_binary_op;
	}
};

} // namespace qasm
} // namespace synthewareQ
