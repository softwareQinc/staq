/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/visitors/generic/base.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "circuits/channel_representation.hpp"

#include <list>
#include <sstream>

namespace synthewareQ {
  using namespace qasm;
  using namespace channel_representation;

  /* Rotation gate merging algorithm based on arXiv:1903.12456 */

  class rotation_folder final : public visitor_base<rotation_folder> {
  public:
    using visitor_base<rotation_folder>::visit;
    friend visitor_base<rotation_folder>;

    rotation_folder() : visitor_base<rotation_folder>() {}
    ~rotation_folder() {}

    void run(ast_context& ctx) { visit(ctx); }

  protected:
    void visit(decl_program* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      fold(accum_);
    }

    void visit(decl_register* node) {}
    void visit(decl_param* node) {}
    void visit(decl_gate* node) {
      // Initialize a new local state
      circuit_callback local_state;
      std::swap(accum_, local_state);

      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      fold(accum_);
      
      std::swap(accum_, local_state);
    }

    /* Statements */
    void visit(stmt_barrier* node) {
    }
    void visit(stmt_cnot* node) {
    }
    void visit(stmt_unitary* node) {
    }
    void visit(stmt_gate* node) {
      printer_.visit(&node->gate());
      auto name = stream_.str();
      clear();

    }
    void visit(stmt_reset* node) {
    }
    void visit(stmt_measure* node) {
    }
    void visit(stmt_if* node) {
    }

    /* Expressions */
    void visit(expr_decl_ref* node) {}
    void visit(expr_reg_idx_ref* node) {}
    void visit(expr_integer* node) {}
    void visit(expr_pi* node) {}
    void visit(expr_real* node) {}
    void visit(expr_binary_op* node) {}
    void visit(expr_unary_op* node) {}

    /* Extensions */
    void visit(decl_oracle*) {}
    void visit(decl_ancilla*) {}

    /* Lists */
    void visit(list_gops* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
    }
    void visit(list_ids* node) {
      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
    }

  private:
    using circuit_callback = std::list<std::pair<ast_node*, channel_op> >;

    // Algorithm state 
    circuit_callback accum_;

    void fold(circuit_callback& circuit) {}

    // String stream and source printer for getting string representations of arguments
    // The use of the source printer is overkill, but it's the easiest and safest way
    std::stringstream stream_;
    source_printer printer_ = source_printer(stream_);

    void clear() { stream_.str(std::string()); }

    std::list<std::string> get_arguments(ast_node* node) {
      std::list<std::string> tmp;

      while (node != nullptr) {
        printer_.visit(node);
        tmp.push_back(stream_.str());
        clear();

        node = node->next_sibling();
      }

      return tmp;
    }
  };

}
