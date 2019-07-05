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

enum class register_type : unsigned short {
	classical = 0,
	quantum = 1,
};

  // This represents a register (quantum or classical) declaration
  class decl_register final : public ast_node {
  private:
    //Configure bits
    enum {
      is_quantum_ = 0
    };
  public:
    static decl_register* build(ast_context* ctx, uint32_t location, register_type type,
                                std::string_view identifier, uint32_t size)
    {
      return new (*ctx) decl_register(location, type, identifier, size);
    }

    ast_node* copy(ast_context* ctx) const
    {
      auto ty = is_quantum() ? register_type::quantum : register_type::classical;
      return build(ctx, location_, ty, identifier_, size_);
    }

    bool is_quantum() const
    {
      return ((this->config_bits_ >> is_quantum_) & 1) == 1;
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
    decl_register(uint32_t location, register_type type, std::string_view identifier,
                  uint32_t size)
      : ast_node(location)
      , size_(size)
      , identifier_(identifier)
    {
      this->config_bits_ |= (static_cast<int>(type) << is_quantum_);
    }

    ast_node_kinds do_get_kind() const override
    {
      return ast_node_kinds::decl_register;
    }

  private:
    uint32_t size_;
	std::string identifier_;
  };

} // namespace qasm
} // namespace synthewareQ
