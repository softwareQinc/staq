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

  // This represents an ancilla (local) register declaration
  class decl_ancilla final : public ast_node {
  private:
    //Configure bits
    enum {
      is_dirty_ = 0
    };
  public:
    static decl_ancilla* build(ast_context* ctx, uint32_t location,
                               std::string_view identifier, uint32_t size, bool dirty)
    {
      return new (*ctx) decl_ancilla(location, identifier, size, dirty);
    }

    ast_node* copy(ast_context* ctx) const
    {
      return build(ctx, location_, identifier_, size_, is_dirty());
    }

    bool is_dirty() const
    {
      return ((this->config_bits_ >> is_dirty_) & 1) == 1;
    }

    std::string_view identifier() const
    {
      return identifier_;
    }

    uint32_t size() const
    {
      return size_;
    }

  private:
    decl_ancilla(uint32_t location, std::string_view identifier, uint32_t size, bool dirty)
      : ast_node(location)
      , size_(size)
      , identifier_(identifier)
    {
      this->config_bits_ |= (static_cast<int>(dirty) << is_dirty_);
    }

    ast_node_kinds do_get_kind() const override
    {
      return ast_node_kinds::decl_ancilla;
    }

  private:
	uint32_t size_;
	std::string identifier_;
  };

} // namespace qasm
} // namespace synthewareQ
