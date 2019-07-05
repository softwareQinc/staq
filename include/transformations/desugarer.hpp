/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"

#include <list>
#include <unordered_map>
#include <variant>

namespace synthewareQ {
namespace transformations {

  using namespace qasm;

  /* \brief! Desugars a QASM AST
   *
   * Visits an AST and replaces all *uniform* gates -- gates
   * applied to a register or registers of qubits at once --
   * with a sequence of individual gate applications
   */
  void desugar(qasm::ast_context*);

  /* Implementation */
  class desugarer final : public replacer<desugarer> {
  public:
    using replacer<desugarer>::visit;

    desugarer(ast_context* ctx) : ctx_(ctx) {}

    // Necessary since replacer is strictly post-order 
    void visit(decl_program* node) override {
      push_scope();
      visit_children(node);
      pop_scope();

      replacement_ = std::nullopt;
    }
    void visit(decl_gate* node) override {
      push_scope();
      visit_children(node);
      pop_scope();

      replacement_ = std::nullopt;
    }

    /* Overrides */
    std::optional<ast_node_list> replace(decl_register* node) override {
      set_var(node->identifier(), { node->size() });
      return std::nullopt;
    }
    std::optional<ast_node_list> replace(decl_param* node) override {
      set_var(node->identifier(), { });
      return std::nullopt;
    }
    std::optional<ast_node_list> replace(decl_ancilla* node) override {
      set_var(node->identifier(), { node->size() });
      return std::nullopt;
    }

    std::optional<ast_node_list> replace(stmt_barrier* node) override {
      // Convert the list_aps to a list of nodes
      // Will fail if the arguments are not grouped into a list_aps
      std::list<ast_node*> args;

      for (auto& arg : *static_cast<list_aps*>(&node->first_arg())) {
        args.push_back(&arg);
      }
      
      if (auto num = repeats(args)) {
        ast_node_list ret;

        // Do the expansion
        for (uint32_t i = 0; i < *num; i++) {
          auto builder = stmt_barrier::builder(ctx_, node->location());

          auto arg_builder = list_aps::builder(ctx_, node->location());
          for (auto arg : args) arg_builder.add_child(expand(arg, i));
          builder.add_child(arg_builder.finish());

          ret.push_back(&node->parent(), builder.finish());
        }

        return ret;
      } else {
        return std::nullopt;
      }
    }

    std::optional<ast_node_list> replace(stmt_cnot* node) override {
      if (auto num = repeats({ &node->control(), &node->target() })) {
        ast_node_list ret;

        // Do the expansion
        for (uint32_t i = 0; i < *num; i++) {
          auto builder = stmt_cnot::builder(ctx_, node->location());

          builder.add_child(expand(&node->control(), i));
          builder.add_child(expand(&node->target(), i));

          ret.push_back(&node->parent(), builder.finish());
        }

        return ret;
      } else {
        return std::nullopt;
      }
    }
    
    std::optional<ast_node_list> replace(stmt_unitary* node) override {
      if (auto num = repeats({ &node->arg() })) {
        ast_node_list ret;

        // Do the expansion
        for (uint32_t i = 0; i < *num; i++) {
          auto builder = stmt_unitary::builder(ctx_, node->location());

          builder.add_child(node->theta().copy(ctx_));
          builder.add_child(node->phi().copy(ctx_));
          builder.add_child(node->lambda().copy(ctx_));
          builder.add_child(expand(&node->arg(), i));

          ret.push_back(&node->parent(), builder.finish());
        }

        return ret;
      } else {
        return std::nullopt;
      }
    }

    std::optional<ast_node_list> replace(stmt_gate* node) override {
      // Convert the list_aps to a list of nodes
      // Will fail if the arguments are not grouped into a list_aps
      std::list<ast_node*> args;

      for (auto& arg : *static_cast<list_aps*>(&node->q_args())) {
        args.push_back(&arg);
      }
      
      if (auto num = repeats(args)) {
        ast_node_list ret;

        // Do the expansion
        for (uint32_t i = 0; i < *num; i++) {
          auto builder = stmt_gate::builder(ctx_, node->location(), node->gate());

          if (node->has_cargs()) builder.add_cargs(node->c_args().copy(ctx_));

          auto arg_builder = list_aps::builder(ctx_, node->location());
          for (auto arg : args) arg_builder.add_child(expand(arg, i));
          builder.add_qargs(arg_builder.finish());

          ret.push_back(&node->parent(), builder.finish());
        }

        return ret;
      } else {
        return std::nullopt;
      }
    }

    std::optional<ast_node_list> replace(stmt_reset* node) override {
      if (auto num = repeats({ &node->arg() })) {
        ast_node_list ret;

        // Do the expansion
        for (uint32_t i = 0; i < *num; i++) {
          auto builder = stmt_reset::builder(ctx_, node->location());

          builder.add_child(expand(&node->arg(), i));

          ret.push_back(&node->parent(), builder.finish());
        }

        return ret;
      } else {
        return std::nullopt;
      }
    }

    std::optional<ast_node_list> replace(stmt_measure* node) override {
      if (auto num = repeats({ &node->quantum_arg(), &node->classical_arg() })) {
        ast_node_list ret;

        // Do the expansion
        for (uint32_t i = 0; i < *num; i++) {
          auto builder = stmt_measure::builder(ctx_, node->location());

          builder.add_child(expand(&node->quantum_arg(), i));
          builder.add_child(expand(&node->classical_arg(), i));

          ret.push_back(&node->parent(), builder.finish());
        }

        return ret;
      } else {
        return std::nullopt;
      }
    }

  private:
    using type_info = std::variant<std::monostate, uint32_t>;

    qasm::ast_context* ctx_;

    // Symbol tracking
    std::list<std::unordered_map<std::string_view, type_info> > symbol_table;

    void push_scope() {
      symbol_table.push_front({ });
    }
      
    void pop_scope() {
      symbol_table.pop_front();
    }

    void set_var(std::string_view x, type_info t) {
      symbol_table.front()[x] = t;
    }

    std::optional<type_info> lookup(std::string_view x) {
      for (auto& table : symbol_table) {
        if (auto it = table.find(x); it != table.end()) {
          return std::make_optional(it->second);
        }
      }

      return std::nullopt;
    }

    // Compute the number of repeats with different offsets for a given list of arguments
    std::optional<uint32_t> repeats(std::list<ast_node*> args) {
      std::optional<uint32_t> ret = std::nullopt;
      
      for (auto arg : args) {
        switch(arg->kind()) {
        case ast_node_kinds::expr_var:
          if (auto ty = lookup(static_cast<expr_var*>(arg)->id())) {
            if (std::holds_alternative<uint32_t>(*ty)) {
              // We assume (wrongfully so atm) that correctness of uniform gates has been checked
              if (!ret) ret = std::get<uint32_t>(*ty);
            }
          }
          break;
        default:
          break;
        }
      }

      return ret;
    }

    // Expand an argument with a given offset if register typed, otherwise copy it
    ast_node* expand(ast_node* arg, uint32_t offset) {
      if (arg->kind() == ast_node_kinds::expr_var) {
        // The semantic analysis should assert that the lookup below will not fail
        auto var = static_cast<expr_var*>(arg);
        auto ty = lookup(var->id());

        if (std::holds_alternative<uint32_t>(*ty)) {
          // It's a register, so dereference it
          auto offset_expr = expr_integer::create(ctx_, var->location(), offset);
          return expr_reg_offset::build(ctx_, var->location(), var->id(), offset_expr);
        }
      }

      return arg->copy(ctx_);
    }

    // Debugging
    void print_current_scope() {
      for (auto& [x, ty] : symbol_table.front()) {
        if (std::holds_alternative<uint32_t>(ty)) {
          std::cout << x << ": Register(" << std::get<uint32_t>(ty) << "), ";
        } else {
          std::cout << x << ": Bit, ";
        }
      }
      std::cout << "\n";
    }

  };
    
  void desugar(qasm::ast_context* ctx) {
    auto trans = desugarer(ctx);
    trans.visit(*ctx);
  }

}
}
