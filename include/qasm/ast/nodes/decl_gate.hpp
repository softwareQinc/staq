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

  enum class gate_type : unsigned short {
    defined = 0,
    opaque = 1,
  };

  // A `decl_gate` node has three children, two of which optional.
  // The children objects are in order:
  //
  // * A `list_ids *` for the parameter list.
  //    Present if and only if has_parameters().
  //
  // * A "list_ids *" for the qubit argument list. (At least one qubit argument is required)
  //    Always present.
  //
  // * A "list_gops *" for the body.
  //    Present if and only if has_body().
  class decl_gate
    : public ast_node
    , public ast_node_container<decl_gate, ast_node> {
  private:
    //Configure bits
    enum {
      has_params_ = 0,
      has_body_  = 1,
    };

  public:
    class builder {
    public:
      explicit builder(ast_context* ctx, uint32_t location, std::string_view identifier,
                       gate_type type = gate_type::defined)
        : statement_(new (*ctx) decl_gate(location, identifier, type))
      {}

      void add_parameters(ast_node* parameters)
      {
        statement_->config_bits_ |= (1 << has_params_);
        statement_->add_child(parameters);
      }

      void add_arguments(ast_node* arguments)
      {
        statement_->add_child(arguments);
      }

      void add_body(ast_node* ops)
      {
        statement_->config_bits_ |= (1 << has_body_);
        statement_->add_child(ops);
      }

      decl_gate* finish()
      {
        return statement_;
      }

    private:
      decl_gate* statement_;
    };
    
    ast_node* copy(ast_context* ctx) const
    {
      auto tmp = builder(ctx, location_, identifier_, has_body() ? gate_type::defined : gate_type::opaque);
      auto it = begin();
      if (has_parameters()) tmp.add_parameters((it++)->copy(ctx));
      tmp.add_arguments((it++)->copy(ctx));
      if (has_body()) tmp.add_body(it->copy(ctx));

      return tmp.finish();
    }


    std::string_view identifier() const
    {
      return identifier_;
    }

    bool has_parameters() const
    {
      return ((this->config_bits_ >> has_params_) & 1) == 1;
    }

    ast_node& parameters()
    {
      return *(this->begin());
    }

    // FIXME: this is hacky, implement random access iterator
    ast_node& arguments()
    {
      auto iter = this->begin();
      if (has_parameters()) {
        ++iter;
      }
      return *iter;
    }

    bool has_body() const
    {
      return ((this->config_bits_ >> has_body_) & 1) == 1;
    }

    ast_node& body()
    {
      auto iter = this->begin();
      if (has_parameters()) {
        ++iter;
      }
      return *(++iter);
    }

  private:
    decl_gate(uint32_t location, std::string_view identifier, gate_type type)
      : ast_node(location)
      , identifier_(identifier)
	{}

	ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::decl_gate;
	}

  private:
	std::string identifier_;
  };

} // namespace qasm
} // namespace synthewareQ
