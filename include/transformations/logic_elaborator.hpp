/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include <tweedledee/qasm/qasm.hpp>
#include <tweedledee/qasm/ast/ast.hpp>
#include <tweedledee/qasm/ast/visitor.hpp>

#include "../synthesis/logic_synthesis.hpp"

namespace synthewareQ {

  namespace qasm = tweedledee::qasm;

  class logic_elaborator : public qasm::visitor_base<logic_elaborator> {
  public:
    logic_elaborator(qasm::ast_context* ctx)
      : visitor_base<logic_elaborator>()
      , ctx_(ctx)
    {}

	void visit_decl_gate(qasm::decl_gate* node)
	{
      if (node->is_classical()) {
        // TODO: fix this bad hack
        visit(&node->file());
        auto l_net = read_from_file(current_filename_);
        auto body = synthesize(ctx_, node->location(), l_net, static_cast<qasm::list_ids*>(&node->arguments()));
        node->set_body(body);
      }
    }

    void visit_logic_file(qasm::logic_file* node)
    {
      current_filename_ = node->filename();
    }

  private:
    qasm::ast_context* ctx_;
    std::string current_filename_;
  };
    
}
