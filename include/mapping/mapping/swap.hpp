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
 * \file mapping/mapping/swap.hpp
 * \brief Local swapping hardware mapper
 */

#include "ast/replacer.hpp"
#include "transformations/substitution.hpp"
#include "mapping/device.hpp"

#include <map>

// TODO: figure out what to do with if statements

namespace staq {
namespace mapping {

/**
 * \class staq::mapping::SwapMapper
 * \brief Simple swap-inserting mapping algorithm
 * \note Assumes the circuit has a single global register with the configured
 * name
 *
 * Maps an AST to a given device by inserting swap gates along a shortest path
 * before each non-local CNOT gate. The mapper keeps track of the current qubit
 * permutation, rather than "swapping back" after each non-local gate.
 */
class SwapMapper final : public ast::Replacer {
  public:
    struct config {
        std::string register_name = "q";
    };

    SwapMapper(Device& device) : Replacer(), device_(device) {
        for (auto i = 0; i < device.qubits_; i++) {
            permutation_[i] = i;
        }
    }

    // Ignore declarations if they were left in during inlining
    void visit(ast::GateDecl&) override {}
    void visit(ast::OracleDecl&) override {}

    std::optional<ast::VarAccess> replace(ast::VarAccess& va) override {
        if (va.var() == config_.register_name)
            return ast::VarAccess(va.pos(), va.var(),
                                  permutation_[*va.offset()]);
        else
            return std::nullopt;
    }

    // Where the magic happens
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::CNOTGate& gate) override {
        // Post order, so the permutation is already applied
        auto ctrl = *(gate.ctrl().offset());
        auto tgt = *(gate.tgt().offset());

        // Compute shortest path
        path cnot_chain = device_.shortest_path(ctrl, tgt);

        if (cnot_chain.empty()) {
            std::cerr << "Error: could not find a connection between qubits "
                      << ctrl << " and " << tgt << "\n";
            return std::nullopt;
        } else {
            std::list<ast::ptr<ast::Gate>> ret;

            // Create a swap chain & update the current permutation
            auto i = ctrl;
            for (auto j : cnot_chain) {
                if (j == tgt) {
                    ret.emplace_back(generate_cnot(i, j, gate.pos()));
                    break;
                } else if (j != i) {
                    // Swap i and j
                    auto swap_i = i;
                    auto swap_j = j;
                    if (!device_.coupled(i, j)) {
                        swap_i = j;
                        swap_j = i;
                    }

                    // CNOT 1
                    ret.emplace_back(generate_cnot(swap_i, swap_j, gate.pos()));

                    // CNOT 2
                    if (device_.coupled(swap_j, swap_i)) {
                        ret.emplace_back(
                            generate_cnot(swap_j, swap_i, gate.pos()));
                    } else {
                        ret.emplace_back(generate_hadamard(swap_i, gate.pos()));
                        ret.emplace_back(generate_hadamard(swap_j, gate.pos()));
                        ret.emplace_back(
                            generate_cnot(swap_i, swap_j, gate.pos()));
                        ret.emplace_back(generate_hadamard(swap_i, gate.pos()));
                        ret.emplace_back(generate_hadamard(swap_j, gate.pos()));
                    }

                    // CNOT 3
                    ret.emplace_back(generate_cnot(swap_i, swap_j, gate.pos()));

                    // Adjust permutation
                    for (auto& [q_init, q] : permutation_) {
                        if (q == i)
                            q = j;
                        else if (q == j)
                            q = i;
                    }
                }
                i = j;
            }
            return std::move(ret);
        }
    }

  private:
    Device device_;
    std::map<int, int> permutation_;
    config config_;

    ast::ptr<ast::CNOTGate> generate_cnot(int i, int j, parser::Position pos) {
        auto ctrl = ast::VarAccess(pos, config_.register_name, i);
        auto tgt = ast::VarAccess(pos, config_.register_name, j);
        return std::make_unique<ast::CNOTGate>(
            ast::CNOTGate(pos, std::move(ctrl), std::move(tgt)));
    }

    ast::ptr<ast::UGate> generate_hadamard(int i, parser::Position pos) {
        auto tgt = ast::VarAccess(pos, config_.register_name, i);

        auto tmp1 = std::make_unique<ast::PiExpr>(ast::PiExpr(pos));
        auto tmp2 = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 2));
        auto theta = std::make_unique<ast::BExpr>(ast::BExpr(
            pos, std::move(tmp1), ast::BinaryOp::Divide, std::move(tmp2)));
        auto phi = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 0));
        auto lambda = std::make_unique<ast::PiExpr>(ast::PiExpr(pos));

        return std::make_unique<ast::UGate>(
            ast::UGate(pos, std::move(theta), std::move(phi), std::move(lambda),
                       std::move(tgt)));
    }
};

/** \brief Applies the swap mapper to an AST given a physical device */
void map_onto_device(Device& device, ast::Program& prog) {
    SwapMapper mapper(device);
    prog.accept(mapper);
}

} // namespace mapping
} // namespace staq
