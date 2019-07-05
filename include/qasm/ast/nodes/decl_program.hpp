/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../ast_node.hpp"
#include "../ast_node_kinds.hpp"

#include <memory>

namespace synthewareQ {
namespace qasm {

// Root node class of the AST
class decl_program final
	: public ast_node
	, public ast_node_container<decl_program, ast_node> {
public:
	class builder {
	public:
		explicit builder()
			: program_(new decl_program())
		{}

		void add_child(ast_node* child)
		{
			program_->add_child(child);
		}

		decl_program& get()
		{
			return *program_;
		}

		decl_program* finish()
		{
			return program_;
		}

	private:
		decl_program* program_;
	};

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder();
      for (auto& child : *this) tmp.add_child(child.copy(ctx));

      return tmp.finish();
    }

private:
	decl_program()
		: ast_node(0)
	{}

	ast_node_kinds do_get_kind() const override
	{
		return ast_node_kinds::decl_program;
	}
};

} // namespace qasm
} // namespace synthewareQ
