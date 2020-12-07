/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2021 softwareQ Inc. All rights reserved.
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
 * \file transformations/barrier_merge.hpp
 * \brief Merge adjacent barriers
 */

#pragma once

#include "ast/traversal.hpp"
#include "ast/replacer.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

namespace staq {
namespace transformations {

/**
 * \brief Merges adjacent barriers
 *
 * Traverses an AST and merges all adjacent barriers.
 */

/* Implementation */
class BarrierMerger final : public ast::Traverse {
  public:
    BarrierMerger() = default;
    ~BarrierMerger() = default;

    std::unordered_map<int, std::list<ast::ptr<ast::Gate>>>
    run(ast::ASTNode& node) {
        reset();
        node.accept(*this);
        clear_barrier();
        return std::move(replacement_list_);
    }

    /* Statements */
    void visit(ast::MeasureStmt& stmt) { clear_barrier(); }
    void visit(ast::ResetStmt& stmt) { clear_barrier(); }
    void visit(ast::IfStmt& stmt) { clear_barrier(); }

    /* Gates */
    void visit(ast::UGate& gate) { clear_barrier(); }
    void visit(ast::CNOTGate& gate) { clear_barrier(); }
    void visit(ast::DeclaredGate& gate) { clear_barrier(); }

    void visit(ast::BarrierGate& gate) {
        uids_.push_back(gate.uid());
        for (auto it = gate.args().begin(); it != gate.args().end(); it++) {
            if (std::find(args_.begin(), args_.end(), *it) == args_.end())
                args_.push_back(*it);
        }
    }

  private:
    std::unordered_map<int, std::list<ast::ptr<ast::Gate>>> replacement_list_;
    std::list<int> uids_;
    std::vector<ast::VarAccess> args_;

    void clear_barrier() {
        for (auto it = uids_.begin(); it != uids_.end(); it++) {
            if (std::next(it) == uids_.end()) {
                // Set the new barrier
                std::list<ast::ptr<ast::Gate>> tmp;
                tmp.emplace_back(
                    new ast::BarrierGate(parser::Position(), std::move(args_)));
                replacement_list_[*it] = std::move(tmp);
            } else {
                // Erase the barrier
                replacement_list_[*it] =
                    std::move(std::list<ast::ptr<ast::Gate>>());
            }
        }

        args_.clear();
        uids_.clear();
    }

    void reset() {
        replacement_list_.clear();
        uids_.clear();
        args_.clear();
    }
};

static void merge_barriers(ast::ASTNode& node) {
    BarrierMerger alg;

    auto res = alg.run(node);
    replace_gates(node, std::move(res));
}

} // namespace transformations
} // namespace staq
