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

  // TODO: separate classical and quantum arguments
  class stmt_gate
    : public ast_node
    , public ast_node_container<stmt_gate, ast_node> {
  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location)
        : statement_(new (*ctx) stmt_gate(location))
      {}

      void add_child(ast_node* child)
      {
        statement_->add_child(child);
      }

      void set_c_args(uint32_t num)
      {
        statement_->num_c_args_ = num;
      }

      stmt_gate* finish()
      {
        return statement_;
      }

    private:
      stmt_gate* statement_;
    };

    ast_node& gate()
    {
      return *(this->begin());
    }

    ast_node& first_c_param()
    {
      auto iter = this->begin();
      return *(++iter);
    }

    ast_node& first_q_param()
    {
      auto iter = this->begin();

      iter++;
      for (auto i = 0; i < num_c_args_; i++) iter++;

      return *iter;
    }

  private:
    stmt_gate(uint32_t location)
      : ast_node(location)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_gate;
	}
  private:
    uint32_t num_c_args_;
  };

} // namespace qasm
} // namespace synthewareQ
