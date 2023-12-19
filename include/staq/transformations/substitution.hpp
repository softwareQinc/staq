/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2023 softwareQ Inc. All rights reserved.
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
 * \file transformations/substitution.hpp
 * \brief Utilities for performing substitutions
 */

#ifndef TRANSFORMATIONS_SUBSTITUTION_HPP_
#define TRANSFORMATIONS_SUBSTITUTION_HPP_

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <variant>

#include "qasmtools/ast/replacer.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;

/**
 * \brief Node replacement visitor with symbol tracking
 *
 * Provides a replacement interface together with information about
 * the bound identifiers. Used to substitute only free variables
 * and for capture-avoiding substitution
 *
 */
class ScopedReplacer : public ast::Replacer {
  public:
    ScopedReplacer() = default;
    ~ScopedReplacer() = default;

    void visit(ast::Program& prog) override {
        push_scope();
        ast::Replacer::visit(prog);
        pop_scope();
    }
    void visit(ast::GateDecl& decl) override {
        push_scope();

        for (auto& param : decl.c_params()) {
            add_to_scope(param);
        }
        for (auto& param : decl.q_params()) {
            add_to_scope(param);
        }
        ast::Replacer::visit(decl);

        pop_scope();
        add_to_scope(decl.id());
    }

    void visit(ast::OracleDecl& decl) override { add_to_scope(decl.id()); }
    void visit(ast::RegisterDecl& decl) override { add_to_scope(decl.id()); }
    void visit(ast::AncillaDecl& decl) override { add_to_scope(decl.id()); }

  protected:
    std::list<std::set<std::string_view>> bound_ = {
        {}}; // The declared identifiers in scope

    // Scoping
    void push_scope() { bound_.push_front({}); }
    void pop_scope() { bound_.pop_front(); }
    void add_to_scope(std::string_view x) {
        auto bound_vars = bound_.front();
        bound_vars.insert(x);
    }
    bool free(std::string_view x) {
        auto bound_vars = bound_.front();
        return bound_vars.find(x) == bound_vars.end();
    }
};

/**
 * \brief Substitutes variables within expressions
 */
class SubstVar final : public ScopedReplacer {
  public:
    SubstVar(std::unordered_map<std::string_view, ast::Expr*>& subst)
        : subst_(subst) {}
    ~SubstVar() = default;

    std::optional<ast::ptr<ast::Expr>> replace(ast::VarExpr& expr) override {
        auto v = expr.var();
        if (free(v) && subst_.find(v) != subst_.end()) {
            return ast::object::clone(*subst_[v]);
        }

        return std::nullopt;
    }

  private:
    std::unordered_map<std::string_view, ast::Expr*> subst_; // The substitution
};

inline void
subst_var_expr(std::unordered_map<std::string_view, ast::Expr*>& subst,
               ast::ASTNode& node) {
    SubstVar alg(subst);
    node.accept(alg);
}

/**
 * \brief Substitutes variables accesses
 *
 * Can be used to substitute a variable or register access
 * with either a variable or register access.
 *
 * Rules: z {y <- x} means x maps to y in z
 *    x    {y <- x} = y
 *    x[i] {y <- x} = y[i]
 *    x    {y[i] <- x} = y[i]
 *    x[i] {y[j] <- x} = y[i+j]
 *    x    {y <- x[i]} = x
 *    x[i] {y <- x[i]} = y
 *    x    {y[j] <- x[i]} = x
 *    x[i] {y[j] <- x[i]} = y[j]
 */
class SubstAP final : public ScopedReplacer {
  public:
    SubstAP(std::unordered_map<ast::VarAccess, ast::VarAccess>& subst)
        : subst_(subst) {}
    ~SubstAP() = default;

    std::optional<ast::VarAccess> replace(ast::VarAccess& va) override {
        auto v = va.var();
        auto offset = va.offset();

        if (free(v)) {
            // Check for a substitution of v[offset] first
            if (auto it = subst_.find(va); it != subst_.end()) {
                return ast::VarAccess(va.pos(), it->second.var(),
                                      it->second.offset());
            } else if (auto it = subst_.find(ast::VarAccess(va.pos(), v));
                       it != subst_.end()) {
                auto vp = it->second;

                if (offset && vp.offset()) {
                    return ast::VarAccess(va.pos(), vp.var(),
                                          *offset + *(vp.offset()));
                } else if (vp.offset()) {
                    return ast::VarAccess(va.pos(), vp.var(), *(vp.offset()));
                } else {
                    return ast::VarAccess(va.pos(), vp.var(), *offset);
                }
            }
        }

        return std::nullopt;
    }

  private:
    std::unordered_map<ast::VarAccess, ast::VarAccess>
        subst_; // The substitution
};

inline void
subst_ap_ap(std::unordered_map<ast::VarAccess, ast::VarAccess>& subst,
            ast::ASTNode& node) {
    SubstAP alg(subst);
    node.accept(alg);
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_SUBSTITUTION_HPP_ */
