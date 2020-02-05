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

#pragma once

/**
 * \file mapping/layout/basic.hpp
 * \brief Simple hardware layout generation
 */

#include "ast/replacer.hpp"
#include "ast/traversal.hpp"
#include "transformations/substitution.hpp"
#include "mapping/device.hpp"

#include <unordered_map>

namespace staq {
namespace mapping {

/** 
 * \class staq::mapping::LayoutTransformer
 * \brief Applies a hardware layout to a circuit
 *
 * Accepts a layout -- that is, a mapping from variable
 * accesses to addresses of physical qubits -- and rewrites
 * the AST so that all variable accesses refer to the
 * relevant address of a global register representing
 * the physical qubits
 */
class LayoutTransformer final : public ast::Replacer {
  public:

    /**
     * \class staq::mapping::LayoutTransformer::config
     * \brief Holds configuration options
     */
    struct config {
        std::string register_name = "q";
    };

    LayoutTransformer() = default;
    LayoutTransformer(const config& params) : Replacer(), config_(params) {}
    ~LayoutTransformer() = default;

    /** \brief Main transformation method */
    void run(ast::Program& prog, const layout& l) {
        // Visit entire program, removing register declarations, then
        // add the physical register & apply substitutions
        prog.accept(*this);

        // Physical register declaration
        prog.body().emplace_front(
            std::make_unique<ast::RegisterDecl>(ast::RegisterDecl(
                prog.pos(), config_.register_name, true, l.size())));

        // Substitution
        std::unordered_map<ast::VarAccess, ast::VarAccess> subst;
        for (auto const& [access, idx] : l) {
            subst.insert({access, ast::VarAccess(parser::Position(),
                                                 config_.register_name, idx)});
        }
        transformations::subst_ap_ap(subst, prog);
    }

    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::RegisterDecl& decl) override {
        if (decl.is_quantum())
            return std::list<ast::ptr<ast::Stmt>>();
        else
            return std::nullopt;
    }

  private:
    config config_;
};

/** 
 * \class staq::mapping::BasicLayout
 * \brief A simple layout generation algorithm
 *
 * Allocates physical qubits on a first-come, first-serve basis
 */
class BasicLayout final : public ast::Traverse {
  public:
    BasicLayout(Device& device) : Traverse(), device_(device) {}
    ~BasicLayout() = default;

    /** \brief Main generation method */
    layout generate(ast::Program& prog) {
        current_ = layout();
        n_ = 0;

        prog.accept(*this);

        return current_;
    }

    void visit(ast::RegisterDecl& decl) override {
        if (decl.is_quantum()) {
            if (n_ + decl.size() <= device_.qubits_) {
                for (auto i = 0; i < decl.size(); i++) {
                    current_[ast::VarAccess(parser::Position(), decl.id(), i)] =
                        n_ + i;
                }
                n_ += decl.size();
            } else {
                throw std::logic_error("Not enough physical qubits");
            }
        }
    }

  private:
    Device device_;
    layout current_;
    size_t n_;
};

/** \brief Rewrites an AST according to a physical layout */
inline void apply_layout(const layout& l, ast::Program& prog) {
    LayoutTransformer alg;
    alg.run(prog, l);
}

/** \brief Generates a layout for a program on a physical device */
inline layout compute_basic_layout(Device& device, ast::Program& prog) {
    BasicLayout gen(device);
    return gen.generate(prog);
}

} // namespace mapping
} // namespace staq
