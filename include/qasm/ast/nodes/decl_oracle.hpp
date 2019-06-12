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

class decl_oracle final
  : public ast_node
  , public ast_node_container<decl_oracle, ast_node> {
public:
  class builder {
  public:
    explicit builder(ast_context* ctx, uint32_t location, std::string_view identifier)
      : statement_(new (*ctx) decl_oracle(location, identifier))
    {}

    void add_arguments(ast_node* arguments)
    {
      statement_->add_child(arguments);
    }

    void add_target(std::string_view filename)
    {
      statement_->filename_ = filename;
    }

    decl_oracle* finish()
    {
      return statement_;
    }

  private:
    decl_oracle* statement_;
  };

  std::string_view identifier() const
  {
    return identifier_;
  }

  ast_node& arguments()
  {
    return *(this->begin());
  }
    
  std::string_view target() const
  {
    return filename_;
  }

private:
	decl_oracle(uint32_t location, std::string_view identifier)
	    : ast_node(location)
	    , identifier_(identifier)
	{}

	ast_node_kinds do_get_kind() const override
	{
      return ast_node_kinds::decl_oracle;
	}

private:
    std::string identifier_;
    std::string filename_;
};

} // namespace qasm
} // namespace synthewareQ
