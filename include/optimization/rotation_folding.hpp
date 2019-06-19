/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "qasm/visitors/generic/base.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "circuits/channel_representation.hpp"

#include <list>
#include <unordered_map>
#include <sstream>

namespace synthewareQ {
  using namespace qasm;
  using namespace channel_representation;

  /* Helper for variant visitor */
  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  /*! \brief Rotation gate merging algorithm based on arXiv:1903.12456 
   *
   *  Returns a replacement list giving the nodes to the be replaced (or erased)
   */

  class rotation_folder final : public visitor_base<rotation_folder> {
  public:
    using visitor_base<rotation_folder>::visit;
    friend visitor_base<rotation_folder>;

    rotation_folder() : visitor_base<rotation_folder>() {}
    ~rotation_folder() {}

    std::unordered_map<ast_node*, ast_node*> run(ast_context& ctx) {
      ctx_ = &ctx;
      visit(ctx);
      return replacement_list_;
    }

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
        if (name == "cx") current_clifford_ *= clifford_op::cnot_gate(*it, *(std::next(it)));
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

    ast_context* ctx_;
    std::unordered_map<ast_node*, ast_node*> replacement_list_;

    /* Algorithm state */
    circuit_callback accum_;       // The currently accumulating circuit (in channel repr.)
    bool mergeable_ = true;        // Whether we're in a context where a gate can be merged
    clifford_op current_clifford_; // The current clifford operator
    // Note: current clifford is stored as the dagger of the actual Clifford gate
    // this is so that left commutation (i.e. conjugation) actually right-commutes
    // the rotation gate, allowing us to walk the circuit forwards rather than backwards
    // That is, we want to end up with the form
    //   C_0R_1R_2R_3...R_n
    // rather than the form
    //   R_n...R_3R_2R_1C_0
    // as in the paper

    /* Phase two of the algorithm */
    td::angle fold(circuit_callback& circuit) {
      // Temporary for debugging
      for (auto& [node, op] : circuit) {
        std::visit([](auto& op) { std::cout << op << "."; }, op);
      }
      std::cout << "\n";

      auto phase = td::angles::zero;
      for (auto it = circuit.rbegin(); it != circuit.rend(); it++) {
        auto &[node, op] = *it;
        if (auto R = std::get_if<rotation_op>(&op)) {
          auto [new_phase, new_R] = fold_forward(circuit, std::next(it), node, *R);

          phase += new_phase;
          if (!(new_R == *R)) {
            std::cout << "    -->Replacing " << *R << " with " << new_R << "\n";
            // TODO: add a proper replacement
            replacement_list_[node] = nullptr;
          }
        }
      }

      return phase;
    }

    std::pair<td::angle, rotation_op>
    fold_forward(circuit_callback& circuit,
                 circuit_callback::reverse_iterator it,
                 ast_node* node,
                 rotation_op op)
    {
      // Tries to commute op backward as much as possible, merging with applicable
      // gates and deleting them as it goes
      // Note: We go backwards so that we only commute **left** past C^*/**right** past C

      auto phase = td::angles::zero;
      bool cont = true;

      for (; cont && it != circuit.rend(); it++) {
        auto visitor = overloaded {
          [this, it, &op, &phase, &circuit](rotation_op& R) {
              auto res = op.try_merge(R);
              if (res.has_value()) {
                auto &[new_phase, new_op] = res.value();
                phase += new_phase;
                op = new_op;

                // Delete R in circuit & the node
                std::cout << "    -->Removing " << R << "\n";
                replacement_list_[it->first] = nullptr;
                circuit.erase(std::next(it).base());

                return false;
              } else if (op.commutes_with(R)) {
                return true;
              } else {
                return false;
              }
            },
            [&op](clifford_op& C) {
              op = op.commute_left(C); return true;
            },
            [&op](uninterp_op& U) {
              if (!op.commutes_with(U)) return false;
              else return true;
            }
        };

        cont = std::visit(visitor, it->second);
      }

      return std::make_pair(phase, op);
    }

    /* Utilities */
    std::stringstream stream_;
    source_printer printer_ = source_printer(stream_);

    void push_uninterp(uninterp_op op) {
      accum_.push_back(std::make_pair(nullptr, current_clifford_));
      accum_.push_back(std::make_pair(nullptr, op));
      // Clear the current clifford
      current_clifford_ = clifford_op();
    }

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
