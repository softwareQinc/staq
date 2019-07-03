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

  class list_exprs
    : public ast_node
    , public ast_node_container<list_exprs, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : node_(new (*ctx) list_exprs(location))
      {}

      void add_child(ast_node* child)
      {
        node_->add_child(child);
      }

      list_exprs& get()
      {
        return *node_;
      }

      list_exprs* finish()
      {
        return node_;
      }

    private:
      list_exprs* node_;
    };

  private:
    list_exprs(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::list_exprs;
	}
  };

} // namespace qasm
} // namespace synthewareQ
