/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include <tweedledee/qasm/qasm.hpp>
#include <tweedledee/qasm/ast/visitor.hpp>


namespace synthewareQ {

  namespace qasm = tweedledee::qasm;

  class logic_elaborator : public qasm::visitor_base<logic_elaborator> {
  public:
    logic_elaborator(int placeholder)
      : visitor_base<logic_elaborator>()
      , param_(placeholder)
    {}

	void visit_decl_gate(qasm::decl_gate* node)
	{
      if (node->is_classical()) {
        visit(&node->file());
      }
	}

    void visit_logic_file(qasm::logic_file* node)
    {
      std::cout << "Visiting file name: " << node->filename() << std::endl;
    }

  private:
    int param_;
  };
    
}
