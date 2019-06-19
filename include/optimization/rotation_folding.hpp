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
      accum_.push_back(std::make_pair(nullptr, current_clifford_));

      fold(accum_);
    }

    void visit(decl_register* node) {}
    void visit(decl_param* node) {}
    void visit(decl_gate* node) {
      // Initialize a new local state
      circuit_callback local_state;
      clifford_op local_clifford;
      std::swap(accum_, local_state);
      std::swap(current_clifford_, local_clifford);

      for (auto& child : *node) visit(const_cast<ast_node*>(&child));
      accum_.push_back(std::make_pair(nullptr, current_clifford_));

      // temporary
      std::cout << node->identifier() << ":\n  ";
      fold(accum_);
      
      std::swap(accum_, local_state);
      std::swap(current_clifford_, local_clifford);
    }

    /* Statements */
    void visit(stmt_barrier* node) {
      auto args = stringify_list(&node->first_arg());
      push_uninterp(uninterp_op(args));
    }
    void visit(stmt_cnot* node) {
      auto ctrl = stringify(&node->control());
      auto tgt = stringify(&node->target());
      if (mergeable_) {
        current_clifford_ *= clifford_op::cnot_gate(ctrl, tgt);
      } else {
        push_uninterp(uninterp_op({ ctrl, tgt }));
      }
    }
    void visit(stmt_unitary* node) {
      auto arg = stringify(&node->arg());
      push_uninterp(uninterp_op({ arg }));
    }
    void visit(stmt_gate* node) {
      // TODO: deal with other standard library operations
      auto name = stringify(&node->gate());
      auto args = stringify_list(&node->first_q_param());

      if (mergeable_) {
        auto it = args.begin();
        if (name == "cx") current_clifford_ *= clifford_op::cnot_gate(*it, *(++it));
        else if (name == "h") current_clifford_ *= clifford_op::h_gate(*it);
        else if (name == "x") current_clifford_ *= clifford_op::x_gate(*it);
        else if (name == "y") current_clifford_ *= clifford_op::y_gate(*it);
        else if (name == "z") current_clifford_ *= clifford_op::z_gate(*it);
        else if (name == "s") current_clifford_ *= clifford_op::sdg_gate(*it);
        else if (name == "sdg") current_clifford_ *= clifford_op::s_gate(*it);
        else if (name == "t") {
          auto gate = rotation_op::t_gate(*it);
          accum_.push_back(std::make_pair(node, gate.commute_left(current_clifford_)));
        } else if (name == "tdg") {
          auto gate = rotation_op::tdg_gate(*it);
          accum_.push_back(std::make_pair(node, gate.commute_left(current_clifford_)));
        } else push_uninterp(uninterp_op(args));
      } else {
        push_uninterp(uninterp_op(args));
      }
    }
    void visit(stmt_reset* node) {
      auto arg = stringify(&node->arg());
      push_uninterp(uninterp_op({ arg }));
    }
    void visit(stmt_measure* node) {
      auto arg = stringify(&node->quantum_arg());
      push_uninterp(uninterp_op({ arg }));
    }
    void visit(stmt_if* node) {
      mergeable_ = false;
      visit(const_cast<ast_node*>(&node->quantum_op()));
      mergeable_ = true;
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
    bool mergeable_ = true;
    // Note: current clifford is stored as the dagger of the actual Clifford gate
    // this is so that left commutation (i.e. conjugation) actually right-commutes
    // the rotation gate, allowing us to walk the circuit forwards rather than backwards
    // That is, we want to end up with the form
    //   C_0R_1R_2R_3...R_n
    // rather than the form
    //   R_n...R_3R_2R_1C_0
    // as in the paper
    clifford_op current_clifford_;

    void push_uninterp(uninterp_op op) {
      accum_.push_back(std::make_pair(nullptr, current_clifford_));
      accum_.push_back(std::make_pair(nullptr, op));
      // Clear the current clifford
      current_clifford_ = clifford_op();
    }

    void fold(circuit_callback& circuit) {
      // Temporary for debugging
      for (auto& [node, op] : circuit) {
        std::visit([](auto& op) { std::cout << op << "."; }, op);
      }
      std::cout << "\n";
    }

    // String stream and source printer for getting string representations of arguments
    // The use of the source printer is overkill, but it's the easiest and safest way
    std::stringstream stream_;
    source_printer printer_ = source_printer(stream_);

    void clear() { stream_.str(std::string()); }

    std::string stringify(ast_node* node) {
      printer_.visit(node);
      auto tmp = stream_.str();
      clear();

      return tmp;
    }

    std::list<std::string> stringify_list(ast_node* node) {
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
