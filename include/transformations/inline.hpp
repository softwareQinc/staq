/*
 * This file is part of staq.
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

/**
 * \file transformations/inline.hpp
 * \brief AST inlining
 */
#pragma once

#include "ast/replacer.hpp"
#include "substitution.hpp"

#include <unordered_map>
#include <set>

namespace staq {
namespace transformations {

/**
 * \brief Inlines gate calls
 *
 * Traverses an AST and inlines all gate calls. By default qelib calls are NOT
 * inlined, but optionally can be. Local ancillas are hoisted to the global
 * level and reused
 */

/* \brief Default overrides */
static const std::set<std::string_view> default_overrides{
    "x",  "y",  "z",  "h",  "s",  "sdg",  "t", "tdg",
    "rx", "ry", "rz", "cz", "cy", "swap", "cx"};

/* Implementation */
class Inliner final : public ast::Replacer {
  public:
    struct config {
        bool keep_declarations = true;
        std::set<std::string_view> overrides = default_overrides;
        std::string ancilla_name = "anc";
    };

    Inliner() = default;
    Inliner(const config& params) : config_(params) {}
    ~Inliner() = default;

    void visit(ast::Program& prog) override {
        ast::Replacer::visit(prog);

        // Max ancillas needed are now known
        if (max_ancilla_ > 0) {
            auto decl = std::make_unique<ast::RegisterDecl>(ast::RegisterDecl(
                prog.pos(), config_.ancilla_name, true, max_ancilla_));
            prog.body().emplace_front(std::move(decl));
        }

        // Final cleanup to remove ancilla declarations outside of function
        // bodies
        cleaner_.set_config(&config_);
        prog.accept(cleaner_);
    }

    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::GateDecl& decl) override {
        // Replacement is post-order, so body should already be inlined

        if (decl.is_opaque()) {
            // Opaque decl, don't inline
            return std::nullopt;
        } else {
            // Record the gate declaration
            auto& tmp = gate_decls_[decl.id()];
            tmp.c_params = decl.c_params();
            tmp.q_params = decl.q_params();
            decl.foreach_stmt(
                [this, &tmp](auto& gate) { tmp.body.push_back(&gate); });
            tmp.ancillas.swap(current_ancillas);

            return std::nullopt;
        }
    }

    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::RegisterDecl& decl) override {
        if (decl.is_quantum()) {
            registers_.push_back(std::make_pair(decl.id(), decl.size()));
        }
        return std::nullopt;
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::AncillaDecl& decl) override {
        current_ancillas.push_back({decl.id(), decl.size(), decl.is_dirty()});
        return std::nullopt;
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {
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
                q_subst.insert(
                    {ast::VarAccess(gate.pos(), it->second.q_params[i]),
                     gate.qarg(i)});
            }

            // For local ancillas
            auto anc_offset = 0;
            auto reg = registers_.begin();
            auto reg_offset = 0;
            for (auto& anc : it->second.ancillas) {
              if (anc.dirty) {
                // Try to find an unused qubit to use as a dirty ancilla
                auto i = 0;

                while (i < anc.size) {
                  if (reg == registers_.end()) {
                    // Switch to clean ancillas
                    q_subst.insert(
                        {ast::VarAccess(gate.pos(), anc.name, i++),
                         ast::VarAccess(gate.pos(),
                                        config_.ancilla_name,
                                        anc_offset++)});

                  } else if (reg_offset >= reg->second) {
                    // Move to the next register
                    reg++;
                    reg_offset = 0;

                  } else {
                    // Check whether this qubit is used in the gate
                    bool used = false;
                    gate.foreach_qarg([&used,&reg,&reg_offset](auto& arg){
                        used = used || (arg.var() == reg->first &&
                                        arg.offset() == reg_offset);
                      });

                    if (!used) {
                      q_subst.insert(
                          {ast::VarAccess(gate.pos(), anc.name, i++),
                           ast::VarAccess(gate.pos(), reg->first, reg_offset)});
                    }

                    reg_offset++;
                  }
                }
              } else {
                q_subst.insert(
                    {ast::VarAccess(gate.pos(), anc.name),
                     ast::VarAccess(gate.pos(),
                                    config_.ancilla_name,
                                    anc_offset)});
                anc_offset += anc.size;
              }
            }
            SubstAP ap_subst(q_subst);

            // Adjust the number of ancillas used
            if (anc_offset > max_ancilla_) {
                max_ancilla_ = anc_offset;
            }

            // Clone & substitute the gate body
            std::list<ast::ptr<ast::Gate>> body;

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
        bool in_decl_ = false;
        config*
            conf_; // An annoying hack to give access to the inliner's configs

      public:
        void set_config(config* conf) { conf_ = conf; }

        void visit(ast::GateDecl& decl) override {
            in_decl_ = true;
            Replacer::visit(decl);
            in_decl_ = false;
        }
        std::optional<std::list<ast::ptr<ast::Stmt>>>
        replace(ast::GateDecl& decl) override {
            if (!conf_->keep_declarations &&
                conf_->overrides.find(decl.id()) == conf_->overrides.end())
                return std::list<ast::ptr<ast::Stmt>>();
            else
                return std::nullopt;
        }
        std::optional<std::list<ast::ptr<ast::Gate>>>
        replace(ast::AncillaDecl&) override {
            if (!in_decl_)
                return std::list<ast::ptr<ast::Gate>>();
            else
                return std::nullopt;
        }
    };

    struct ancilla_info {
        ast::symbol name;
        int size;
        bool dirty;
    };

    struct gate_info {
        std::vector<ast::symbol> c_params;
        std::vector<ast::symbol> q_params;
        std::list<ast::Gate*> body;
        std::list<ancilla_info> ancillas;
    };

    config config_;
    std::unordered_map<std::string_view, gate_info> gate_decls_;
    Cleaner cleaner_;
    int max_ancilla_ = 0;
    std::list<std::pair<ast::symbol, int>> registers_;

    // Gate-local accumulating values
    std::list<ancilla_info> current_ancillas;
    int num_ancilla = 0;
};

static void inline_ast(ast::ASTNode& node) {
    Inliner alg;
    node.accept(alg);
}

static void inline_ast(ast::ASTNode& node, const Inliner::config& params) {
    Inliner alg(params);
    node.accept(alg);
}

} // namespace transformations
} // namespace staq
