/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2022 softwareQ Inc. All rights reserved.
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
 * \file transformations/oracle_synthesizer.hpp
 * \brief Elaboration of oracles into regular gates
 */

#pragma once

#include "qasmtools/ast/replacer.hpp"
#include "synthesis/logic_synthesis.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;

/**
 * \brief Synthesizes all declared oracles over standard library gates
 *
 * Visits an AST and synthesizes any declared oracles,
 * replacing them with regular gate declarations which may
 * optionally declare local ancillas
 */

/* Implementation */
class OracleSynthesizer final : public ast::Replacer {
  public:
    OracleSynthesizer() = default;
    ~OracleSynthesizer() = default;

    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::OracleDecl& decl) {
        auto l_net = synthesis::read_network(decl.fname());
        auto body = synthesis::synthesize_net(decl.pos(), l_net, decl.params());

        std::list<ast::ptr<ast::Stmt>> ret;
        ret.emplace_back(std::make_unique<ast::GateDecl>(ast::GateDecl(
            decl.pos(), decl.id(), false, {}, decl.params(), std::move(body))));
        return std::move(ret);
    }
};

void synthesize_oracles(ast::ASTNode& node) {
    OracleSynthesizer alg;
    node.accept(alg);
}

} // namespace transformations
} // namespace staq
