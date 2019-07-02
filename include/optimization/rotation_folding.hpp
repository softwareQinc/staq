/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "qasm/visitors/generic/base.hpp"
#include "qasm/visitors/generic/replacer.hpp"
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

  // TODO: Add possibility for global phase correction
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
      accum_.push_back(current_clifford_);

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
      accum_.push_back(current_clifford_);

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
          rotation_info info = { node, rotation_info::axis::z, &node->first_q_param() };
          
          accum_.push_back(std::make_pair(info, gate.commute_left(current_clifford_)));
        } else if (name == "tdg") {
          auto gate = rotation_op::tdg_gate(*it);
          rotation_info info = { node, rotation_info::axis::z, &node->first_q_param() };

          accum_.push_back(std::make_pair(info, gate.commute_left(current_clifford_)));
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
    // Store information necessary for generating a replacement of <node> with a
    // different angle
    struct rotation_info {
      enum class axis { x, y, z};
      
      ast_node* node;
      axis rotation_axis;
      ast_node* arg;
    };

    using circuit_callback =
      std::list<std::variant<uninterp_op, clifford_op, std::pair<rotation_info, rotation_op> > >;

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
      auto phase = td::angles::zero;

      for (auto it = circuit.rbegin(); it != circuit.rend(); it++) {
        auto& op = *it;
        if (auto tmp = std::get_if<std::pair<rotation_info, rotation_op> >(&op)) {
          auto [new_phase, new_R] = fold_forward(circuit, std::next(it), tmp->second);

          phase += new_phase;
          if (!(new_R == tmp->second)) {
            replacement_list_[tmp->first.node] = new_rotation(tmp->first, new_R.rotation_angle());
          }
        }
      }

      return phase;
    }

    std::pair<td::angle, rotation_op>
    fold_forward(circuit_callback& circuit, circuit_callback::reverse_iterator it, rotation_op R)
    {
      // Tries to commute op backward as much as possible, merging with applicable
      // gates and deleting them as it goes
      // Note: We go backwards so that we only commute **left** past C^*/**right** past C

      auto phase = td::angles::zero;
      bool cont = true;

      for (; cont && it != circuit.rend(); it++) {
        auto visitor = overloaded {
          [this, it, &R, &phase, &circuit](std::pair<rotation_info, rotation_op>& Rop) {
              auto res = R.try_merge(Rop.second);
              if (res.has_value()) {
                auto &[new_phase, new_R] = res.value();
                phase += new_phase;
                R = new_R;

                // Delete R in circuit & the node
                replacement_list_[Rop.first.node] = nullptr;
                circuit.erase(std::next(it).base());

                return false;
              } else if (R.commutes_with(Rop.second)) {
                return true;
              } else {
                return false;
              }
            },
            [&R](clifford_op& C) {
              R = R.commute_left(C); return true;
            },
            [&R](uninterp_op& U) {
              if (!R.commutes_with(U)) return false;
              else return true;
            }
        };

        cont = std::visit(visitor, *it);
      }

      return std::make_pair(phase, R);
    }

    /* Utilities */
    std::stringstream stream_;
    source_printer printer_ = source_printer(stream_);

    void push_uninterp(uninterp_op op) {
      accum_.push_back(current_clifford_);
      accum_.push_back(op);
      // Clear the current clifford
      current_clifford_ = clifford_op();
    }

    /*! \brief Returns a qasm expr node with the value of the given angle */
    // TODO: Find a better home for this, it's duplicated in logic synthesis
    ast_node* angle_to_expr(ast_context* ctx_, uint32_t location, tweedledum::angle theta) {
      auto sval = theta.symbolic_value();
      if (sval == std::nullopt) {
        // Angle is real-valued
        return expr_real::create(ctx_, location, theta.numeric_value());
      } else {
        // Angle is of the form pi*(a/b) for a & b integers
        // NOTE: tweedledum::gate base and tweedledum::angle contradict -- the former defines t as
        //       an angle of 1/4, while the latter gives it as an angle of 1/8.
        auto [a, b] = sval.value();

        if (a == 0) {
          return expr_integer::create(ctx_, location, 0);
        } else if (a == 1) {
          auto total = expr_binary_op::builder(ctx_, location, binary_ops::division);
          total.add_child(expr_pi::create(ctx_, location));
          total.add_child(expr_integer::create(ctx_, location, b));

          return total.finish();
        } else if (a == -1) {
          auto numerator = expr_unary_op::builder(ctx_, location, unary_ops::minus);
          numerator.add_child(expr_pi::create(ctx_, location));

          auto total = expr_binary_op::builder(ctx_, location, binary_ops::division);
          total.add_child(numerator.finish());
          total.add_child(expr_integer::create(ctx_, location, b));

          return total.finish();
        } else {
          auto numerator = expr_binary_op::builder(ctx_, location, binary_ops::multiplication);
          numerator.add_child(expr_integer::create(ctx_, location, a));
          numerator.add_child(expr_pi::create(ctx_, location));

          auto total = expr_binary_op::builder(ctx_, location, binary_ops::division);
          total.add_child(numerator.finish());
          total.add_child(expr_integer::create(ctx_, location, b));

          return total.finish();
        }
      }
    }

    // Assumes basic gates (x, y, z, s, sdg, t, tdg, rx, ry, rz) are defined
    ast_node* new_rotation(const rotation_info& rinfo, const td::angle& theta) {
      auto location = rinfo.node->location();
      auto stmt_builder = stmt_gate::builder(ctx_, location);

      // Determine the gate
      uint32_t num_c_args = 0;
      auto sval = theta.symbolic_value();
      if (sval == std::nullopt) {
        // Angle is real-valued
        auto val = theta.numeric_value();

        if (val == 0) return nullptr;

        ast_node* decl_ref;
        switch (rinfo.rotation_axis) {
        case rotation_info::axis::x:
          decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("rx"));
          break;
        case rotation_info::axis::y:
          decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("ry"));
          break;
        case rotation_info::axis::z:
          decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("rz"));
          break;
        }
        stmt_builder.add_child(decl_ref);

        auto expr_angle = angle_to_expr(ctx_, location, theta);
        stmt_builder.add_child(expr_angle);
        num_c_args++;
      } else {
        // Angle is of the form pi*(a/b) for a & b integers
        auto [num, denom] = sval.value();

        if (num == 0) return nullptr;
        switch (rinfo.rotation_axis) {
        case rotation_info::axis::x:
          if ((num == 1) && (denom == 1)) {
            // X gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("x"));
            stmt_builder.add_child(decl_ref);
          } else {
            // Rx gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("rx"));
            stmt_builder.add_child(decl_ref);

            auto expr_angle = angle_to_expr(ctx_, location, theta);
            stmt_builder.add_child(expr_angle);
            num_c_args++;
          }
          break;
        case rotation_info::axis::y:
          if ((num == 1) && (denom == 1)) {
            // Y gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("y"));
            stmt_builder.add_child(decl_ref);
          } else {
            // Ry gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("ry"));
            stmt_builder.add_child(decl_ref);

            auto expr_angle = angle_to_expr(ctx_, location, theta);
            stmt_builder.add_child(expr_angle);
            num_c_args++;
          }
          break;
        case rotation_info::axis::z:
          if ((num == 1) && (denom == 1)) {
            // Z gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("z"));
            stmt_builder.add_child(decl_ref);
          } else if (((num == 1) || (num == -3)) && (denom == 2)) {
            // S gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("s"));
            stmt_builder.add_child(decl_ref);
          } else if (((num == -1) || (num == 3)) && (denom == 2)) {
            // Sdg gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("sdg"));
            stmt_builder.add_child(decl_ref);
          } else if (((num == 1) || (num == -7)) && (denom == 4)) {
            // T gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("t"));
            stmt_builder.add_child(decl_ref);
          } else if (((num == -1) || (num == 7)) && (denom == 4)) {
            // Tdg gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("tdg"));
            stmt_builder.add_child(decl_ref);
          } else {
            // Rz gate
            auto decl_ref = expr_decl_ref::build(ctx_, location, ctx_->find_declaration("rz"));
            stmt_builder.add_child(decl_ref);

            auto expr_angle = angle_to_expr(ctx_, location, theta);
            stmt_builder.add_child(expr_angle);
            num_c_args++;
          }
          break;
        }
      }
      stmt_builder.set_c_args(num_c_args);

      // Create a new decl ref for the argument
      ast_node* arg_copy;
      expr_decl_ref* decl;
      switch(rinfo.arg->kind()) {
      case ast_node_kinds::expr_decl_ref:
        decl = static_cast<expr_decl_ref*>(rinfo.arg);
        
        arg_copy = expr_decl_ref::build(ctx_, location, decl->declaration());
        break;
      case ast_node_kinds::expr_reg_idx_ref:
        auto expr = static_cast<expr_reg_idx_ref*>(rinfo.arg);

        auto arg_builder = expr_reg_idx_ref::builder(ctx_, location);

        if (expr->var().kind() != ast_node_kinds::expr_decl_ref) {
          throw std::logic_error("AST invalid -- deference of non declared variable");
        }
        decl = static_cast<expr_decl_ref*>(&expr->var());
        arg_builder.add_child(expr_decl_ref::build(ctx_, location, decl->declaration()));

        if (expr->index().kind() != ast_node_kinds::expr_integer) {
          throw std::logic_error("AST invalid -- deference offset not an integer");
        }
        auto index = static_cast<expr_integer*>(&expr->index());
        arg_builder.add_child(expr_integer::create(ctx_, location, index->evaluate()));
        
        arg_copy = arg_builder.finish();
        break;
      }
      stmt_builder.add_child(arg_copy);

      return stmt_builder.finish();
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

  void rotation_fold(ast_context& ctx) {
    rotation_folder alg;

    auto res = alg.run(ctx);
    bulk_replace(ctx, res);
  }

}
