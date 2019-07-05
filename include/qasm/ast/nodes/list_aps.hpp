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

  class list_aps
    : public ast_node
    , public ast_node_container<list_aps, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : node_(new (*ctx) list_aps(location))
      {}

      void add_child(ast_node* child)
      {
        node_->add_child(child);
      }

      list_aps& get()
      {
        return *node_;
      }

      list_aps* finish()
      {
        return node_;
      }

    private:
      list_aps* node_;
    };

    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_);
      for (auto& child : *this) tmp.add_child(child.copy(ctx));
      
      return tmp.finish(); 
    }

  private:
    list_aps(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::list_aps;
	}
  };

} // namespace qasm
} // namespace synthewareQ
