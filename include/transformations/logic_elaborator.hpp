/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"

#include "synthesis/logic_synthesis.hpp"

namespace synthewareQ {

  class logic_elaborator final : public qasm::replacer<logic_elaborator> {
  public:
    using replacer<logic_elaborator>::visit;
    
    logic_elaborator(qasm::ast_context* ctx)
      : replacer<logic_elaborator>()
      , ctx_(ctx)
    {}
    ~logic_elaborator() {}

    qasm::ast_node* replace(qasm::decl_oracle* node)
	{
      auto decl_builder = qasm::decl_gate::builder(ctx_, node->location(), node->identifier());
      decl_builder.add_arguments(&node->arguments());

      auto l_net = read_from_file(node->target());
      auto body = synthesize(ctx_, node->location(), l_net, static_cast<qasm::list_ids*>(&node->arguments()));
      decl_builder.add_body(body);

      return decl_builder.finish();
    }

  private:
    qasm::ast_context* ctx_;
  };
    
}
