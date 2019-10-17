/*
 * This file is part of synthewareQ.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "ast/replacer.hpp"
#include "substitution.hpp"

#include <unordered_map>
#include <set>

namespace synthewareQ {
namespace transformations {

  /**
   * \brief Inlines gate calls
   *
   * Traverses an AST and inlines all gate calls. By default qelib calls are NOT
   * inlined, but optionally can be. Local ancillas are hoisted to the global
   * level and reused
   */

  /* \brief Default overrides */
  static const std::set<std::string_view> default_overrides {
    "x", "y", "z", "h", "s", "sdg", "t", "tdg", "rx", "ry", "rz",
    "cz", "cy", "swap", "cx"
  };

  /* Implementation */
  class Inliner final : public ast::Replacer {
  public:
    struct config {
      bool keep_declarations = true;
      std::set<std::string_view> overrides = default_overrides;
      std::string ancilla_name = "auto_anc";
    };

    Inliner() = default;
    Inliner(const config& params) : config_(params) {}
    ~Inliner() = default;

    void visit(ast::Program& prog) override {
      ast::Replacer::visit(prog);

      // Max ancillas needed are now known
      if (max_ancilla > 0) {
        auto decl = std::make_unique<ast::RegisterDecl>(ast::RegisterDecl(
          prog.pos(), config_.ancilla_name, true, max_ancilla));
        prog.body().emplace_front(std::move(decl));
      }

      // Final cleanup to remove ancilla declarations outside of function bodies
      prog.accept(cleaner_);
    }

    std::optional<std::list<ast::ptr<ast::Stmt> > > replace(ast::GateDecl& decl) override {
      // Replacement is post-order, so body should already be inlined

      if (decl.is_opaque()) {
        // Opaque decl, don't inline
        return std::nullopt;
      } else {
        // Record the gate declaration
        auto& tmp = gate_decls_[decl.id()];
        tmp.c_params = decl.c_params();
        tmp.q_params = decl.q_params();
        decl.foreach_stmt([this, &tmp](auto& gate){ tmp.body.push_back(&gate); });
        tmp.ancillas.swap(current_ancillas);

        // Retrieve and reset the local ancilla counter
        if (num_ancilla > max_ancilla) {
          max_ancilla = num_ancilla;
        }
        num_ancilla = 0;

        // Keep or delete the declaration
        if (config_.keep_declarations) {
          return std::nullopt;
        } else {
          return std::list<ast::ptr<ast::Stmt> >();
        }
      }
    }

    std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::AncillaDecl& decl) override {
      if (decl.is_dirty()) {
        std::cerr << "Error: dirty ancillas not currently supported by inliner\n";
      } else {
        current_ancillas.push_back(std::make_pair(decl.id(), decl.size()));
        num_ancilla += decl.size();
      }
      return std::nullopt;
    }

    std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::DeclaredGate& gate) override {
      if (config_.overrides.find(gate.name()) != config_.overrides.end()) {
        return std::nullopt;
      }
      
      if (auto it = gate_decls_.find(gate.name()); it != gate_decls_.end()) {
        // Substitute classical arguments
        std::unordered_map<std::string_view, ast::Expr*> c_subst;
        for (auto i = 0; i < gate.num_cargs(); i++) {
          c_subst[it->second.c_params[i]] = &gate.carg(i);
        }
        SubstVar var_subst(c_subst);

        // Substitute quantum arguments
        std::unordered_map<ast::VarAccess, ast::VarAccess> q_subst;
        for (auto i = 0; i < gate.num_qargs(); i++) {
          q_subst.insert({ast::VarAccess(gate.pos(), it->second.q_params[i]), gate.qarg(i)});
        }
        // For local ancillas
        auto offset = 0;
        for (auto& [id, num] : it->second.ancillas) {
          q_subst.insert({ast::VarAccess(gate.pos(), id),
                          ast::VarAccess(gate.pos(), config_.ancilla_name, offset)});
          offset += num;
        }
        SubstAP ap_subst(q_subst);

        // Clone & substitute the gate body
        std::list<ast::ptr<ast::Gate> > body;

        for (auto gate : it->second.body) {
          auto new_gate = gate->clone();
          new_gate->accept(var_subst);
          new_gate->accept(ap_subst);
          body.emplace_back(ast::ptr<ast::Gate>(new_gate));
        }

        return std::move(body);
      } else {
        return std::nullopt;
      }
    }

  private:
    /* Helper class */
    class Cleaner final : public Replacer {
    public:
      void visit(ast::GateDecl& decl) override { } // Don't descend into gate declarations
      std::optional<std::list<ast::ptr<ast::Gate> > > replace(ast::AncillaDecl&) override {
        return std::list<ast::ptr<ast::Gate> >();
      }
    };

    struct gate_info {
      std::vector<ast::symbol> c_params;
      std::vector<ast::symbol> q_params;
      std::list<ast::Gate*> body;
      std::list<std::pair<ast::symbol, int> > ancillas;
    };
    
    config config_;
    std::unordered_map<std::string_view, gate_info> gate_decls_;
    Cleaner cleaner_;
    int max_ancilla = 0;

    // Gate-local accumulating values
    std::list<std::pair<ast::symbol, int> > current_ancillas;
    int num_ancilla = 0;
  };

  void inline_ast(ast::ASTNode& node) {
    Inliner alg;
    node.accept(alg);
  }

  void inline_ast(ast::ASTNode& node, const Inliner::config& params) {
    Inliner alg(params);
    node.accept(alg);
  }

}
}
