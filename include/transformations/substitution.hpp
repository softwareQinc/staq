/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

#include "qasm/ast/ast.hpp"
#include "qasm/visitors/generic/replacer.hpp"

#include <list>
#include <unordered_map>
#include <set>
#include <variant>
#include <map>

namespace synthewareQ {
namespace transformations {

  using namespace qasm;

  using ap = std::variant<std::string_view, std::pair<std::string_view, uint32_t> >;

  /* Helper for variant visitors */
  template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
  template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

  /* \brief! Virtual base class for performing replacements with scope information */
  template<typename Derived>
  class scoped_replacer : public replacer<Derived> {
  public:
    using replacer<Derived>::visit;
    using replacer<Derived>::visit_children;

    scoped_replacer(ast_context* ctx) : ctx_(ctx) {}
    ~scoped_replacer() {};

    void visit(decl_program* node) override {
      push_scope();
      visit_children(node);
      pop_scope();
    }
    void visit(decl_gate* node) override {
      push_scope();
      visit_children(node);
      pop_scope();

      add_to_scope(node->identifier());
    }

    void visit(decl_oracle* node) override   { add_to_scope(node->identifier()); }
    void visit(decl_register* node) override { add_to_scope(node->identifier()); }
    void visit(decl_param* node) override    { add_to_scope(node->identifier()); }
    void visit(decl_ancilla* node) override  { add_to_scope(node->identifier()); }

  protected:
    ast_context* ctx_;

    std::list<std::set<std::string_view> > bound_ = { { } };  // The declared identifiers in scope

    // Scoping
    void push_scope() {
      bound_.push_front({ });
    }
    void pop_scope() {
      bound_.pop_front();
    }
    void add_to_scope(std::string_view x) {
      auto bound_vars = bound_.front();
      bound_vars.insert(x);
    }
    bool free(std::string_view x) {
      auto bound_vars = bound_.front();
      return bound_vars.find(x) == bound_vars.end();
    }
  };

  /* \brief! Applies a variable->node substitution to an AST
   *
   * Given a partial map from identifiers to ast nodes,
   * replaces each identifier in the outer-most scope
   * with its mapping, if it exists. Used to implement
   * substitution & mapping to physical qubits
   *
   * Generally doesn't do sanity checks to ensure the
   * substituted node is in fact an access path
   */
  class variable_substitutor final : public scoped_replacer<variable_substitutor> {
  public:
    using scoped_replacer<variable_substitutor>::visit;

    variable_substitutor(ast_context* ctx) : scoped_replacer(ctx) {}
    ~variable_substitutor() {}

    void subst(std::unordered_map<std::string_view, ast_node*> &substs, ast_node* node) {
      subst_ = substs;
      visit(node);
    }

    std::optional<ast_node_list> replace(expr_var* node) override {
      auto v = node->id();
      if (free(v) && subst_.find(v) != subst_.end()) {
        auto ret = ast_node_list();
        ret.push_back(&node->parent(), subst_[v]->copy(ctx_));
        return ret;
      }

      return std::nullopt;
    }

    std::optional<ast_node_list> replace(expr_reg_offset* node) override {
      auto v = node->id();
      if (free(v) && subst_.find(v) != subst_.end()) {
        auto sub = subst_[v];
        auto ret = ast_node_list();
        // to avoid cross initialization in switch
        expr_integer* offset = nullptr;
        expr_reg_offset* deref = nullptr;
        std::string_view new_v;
        uint32_t new_index;

        switch(sub->kind()) {
        case ast_node_kinds::expr_var:
          // Replace the root variable
          new_v = (static_cast<expr_var*>(sub))->id();
          offset = static_cast<expr_integer*>(node->index().copy(ctx_));
          deref = expr_reg_offset::build(ctx_, node->location(), new_v, offset);
          ret.push_back(&node->parent(), deref);
          return ret;
        case ast_node_kinds::expr_reg_offset:
          // Replace the root variable and add the offsets
          new_v = (static_cast<expr_reg_offset*>(sub))->id();
          offset = expr_integer::create(ctx_, node->location(), node->index_numeric() +
                                        (static_cast<expr_reg_offset*>(sub))->index_numeric());
          deref = expr_reg_offset::build(ctx_, node->location(), new_v, offset);
          ret.push_back(&node->parent(), deref);
          return ret;
        default:
          std::cerr << "Error: Invalid substitution\n";
          return std::nullopt;
        }
      }
    }

  private:
    std::unordered_map<std::string_view, ast_node*> subst_; // The substitution
  };


  /* \brief! Applies an access path substitution to an AST
   *
   * Can be used to substitute a variable or register access
   * with either a variable or register access.
   *
   * Rules: {y <- x} means x maps to y
   *    x    {y <- x} = y
   *    x[i] {y <- x} = y[i]
   *    x    {y[i] <- x} = y[i]
   *    x[i] {y[j] <- x} = y[i+j]
   *    x    {y <- x[i]} = x
   *    x[i] {y <- x[i]} = y
   *    x    {y[j] <- x[i]} = x
   *    x[i] {y[j] <- x[i]} = y[j]
   */
  class ap_substitutor final : public scoped_replacer<ap_substitutor> {
  public:
    using scoped_replacer<ap_substitutor>::visit;

    ap_substitutor(ast_context* ctx) : scoped_replacer(ctx) {}
    ~ap_substitutor() {}

    void subst(std::map<ap, ap> &substitution, ast_node* node) {
      subst_ = substitution;
      visit(node);
    }

    std::optional<ast_node_list> replace(expr_var* node) override {
      auto v = node->id();

      if (free(v) && subst_.find(ap(v)) != subst_.end()) {
        auto ret = ast_node_list();
        ret.push_back(&node->parent(), generate_node(subst_[ap(v)]));
        return ret;
      }

      return std::nullopt;
    }

    std::optional<ast_node_list> replace(expr_reg_offset* node) override {
      auto v = node->id();
      auto offset = (static_cast<expr_integer&>(node->index())).evaluate();

      if (free(v)) {
        // Check for a substitution of v[offset] first
        if (auto it = subst_.find(ap(std::make_pair(v, offset))); it != subst_.end()) {
          auto ret = ast_node_list();
          ret.push_back(&node->parent(), generate_node(it->second));
          return ret;

        } else if (auto it = subst_.find(ap(v)); it != subst_.end()) {
          auto ret = ast_node_list();
          ast_node* tmp;
          std::pair<std::string_view, uint32_t> deref; // To avoid cross initialization

          switch(it->second.index()) {
          case 0: // x[i] {y <- x}
            tmp = generate_node(ap(std::make_pair(std::get<std::string_view>(it->second), offset)));
          case 1: // x[i] {y[j] <- x}
            deref = std::get<std::pair<std::string_view, uint32_t> >(it->second);
            tmp = generate_node(ap(std::make_pair(deref.first, offset + deref.second)));
          }
            
          ret.push_back(&node->parent(), tmp);
          return ret;
        }
      }

      return std::nullopt;
    }

  private:
    std::map<ap, ap> subst_; // The substitution

    ast_node* generate_node(ap access_path) {
      auto visitor = overloaded {
        [this](std::string_view v) { return static_cast<ast_node*>(expr_var::build(ctx_, 0, v)); },
        [this](std::pair<std::string_view, uint32_t> v_off) {
          auto tmp = expr_reg_offset::build(ctx_, 0, v_off.first, expr_integer::create(ctx_, 0, v_off.second));
          return static_cast<ast_node*>(tmp);
        }};
      return std::visit(visitor, access_path);
    }

  };

}
}
