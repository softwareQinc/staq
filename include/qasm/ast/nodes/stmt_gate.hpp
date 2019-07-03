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
      explicit builder(ast_context* ctx, uint32_t location, std::string_view gate)
        : statement_(new (*ctx) stmt_gate(location, gate))
      {}

      void add_cargs(ast_node* cargs)
      {
        statement_->add_child(cargs);
        statement_->has_cargs_ = true;
      }

      void add_qargs(ast_node* qargs)
      {
        statement_->add_child(qargs);
      }

      stmt_gate* finish()
      {
        return statement_;
      }

    private:
      stmt_gate* statement_;
    };

    std::string_view gate() const {
      return name_;
    }

    bool has_cargs() const
    {
      return has_cargs_;
    }

    ast_node& c_args()
    {
      assert(has_cargs_);
      return *(this->begin());
    }

    ast_node& q_args()
    {
      if (has_cargs_) {
        return *(std::next(this->begin()));
      } else {
        return *(this->begin());
      }
    }

  private:
    stmt_gate(uint32_t location, std::string_view gate)
      : ast_node(location)
      , name_(gate)
    {}

    ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::stmt_gate;
	}
  private:
    std::string name_;
    bool has_cargs_ = false;
  };

} // namespace qasm
} // namespace synthewareQ
