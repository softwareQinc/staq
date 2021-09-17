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
 * \file optimization/cnot_resynthesis.hpp
 * \brief CNOT re-synthesis based on Gray-Synth
 */

#pragma once

#include <qasm/ast/visitor.hpp>
#include <qasm/ast/replacer.hpp>
#include "synthesis/cnot_dihedral.hpp"

#include <cstddef>
#include <list>
#include <sstream>
#include <unordered_map>

using namespace qasm;

namespace staq {
namespace optimization {

/**
 * \class staq::optimization::CNOTResynthesizer
 * \brief CNOT optimization algorithm based on arXiv:1712.01859
 */
class CNOTOptimizer final : public ast::Replacer {
  public:
    struct config {};

    CNOTOptimizer() = default;
    CNOTOptimizer(const config& params) : Replacer(), config_(params) {}
    ~CNOTOptimizer() = default;

    void run(ast::ASTNode& node) {
        reset();
        node.accept(*this);
    }

    /* Statements */
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::MeasureStmt& stmt) override {
        auto tmp = flush<ast::Stmt>();
        tmp.emplace_back(ast::ptr<ast::Stmt>(stmt.clone()));
        return std::move(tmp);
    }
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::ResetStmt& stmt) override {
        auto tmp = flush<ast::Stmt>();
        tmp.emplace_back(ast::ptr<ast::Stmt>(stmt.clone()));
        return std::move(tmp);
    }
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::IfStmt& stmt) override {
        auto tmp = flush<ast::Stmt>();
        tmp.emplace_back(ast::ptr<ast::Stmt>(stmt.clone()));
        return std::move(tmp);
    }

    /* Gates */
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::UGate& gate) override {
        if (is_zero(gate.theta()) && is_zero(gate.phi())) {
            // It's a z-axis rotation
            auto idx = get_index(gate.arg());

            // Add the phase
            add_phase(permutation_[idx],
                      ast::ptr<ast::Expr>(gate.lambda().clone()));

            // Delete the gate
            return std::list<ast::ptr<ast::Gate>>();
        } else {
            auto tmp = flush<ast::Gate>();
            tmp.emplace_back(ast::ptr<ast::Gate>(gate.clone()));
            return std::move(tmp);
        }
    }
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::CNOTGate& gate) override {
        auto ctrl = get_index(gate.ctrl());
        auto tgt = get_index(gate.tgt());

        // Add the CNOT
        synthesis::operator^=(permutation_[tgt], permutation_[ctrl]);

        // Delete the gate
        return std::list<ast::ptr<ast::Gate>>();
    }
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::BarrierGate& gate) override {
        auto tmp = flush<ast::Gate>();
        tmp.emplace_back(ast::ptr<ast::Gate>(gate.clone()));
        return std::move(tmp);
    }
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {
        auto name = gate.name();

        if (name == "rz" || name == "u1") {
            auto idx = get_index(gate.qarg(0));
            add_phase(permutation_[idx],
                      ast::ptr<ast::Expr>(gate.carg(0).clone()));

            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "cx") {
            auto ctrl = get_index(gate.qarg(0));
            auto tgt = get_index(gate.qarg(1));

            synthesis::operator^=(permutation_[tgt], permutation_[ctrl]);
            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "z") {
            auto idx = get_index(gate.qarg(0));

            add_phase(permutation_[idx], ast::angle_to_expr(utils::angles::pi));
            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "s") {
            auto idx = get_index(gate.qarg(0));

            add_phase(permutation_[idx],
                      ast::angle_to_expr(utils::angles::pi_half));
            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "sdg") {
            auto idx = get_index(gate.qarg(0));

            add_phase(permutation_[idx],
                      ast::angle_to_expr(-utils::angles::pi_half));
            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "t") {
            auto idx = get_index(gate.qarg(0));

            add_phase(permutation_[idx],
                      ast::angle_to_expr(utils::angles::pi_quarter));
            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "tdg") {
            auto idx = get_index(gate.qarg(0));

            add_phase(permutation_[idx],
                      ast::angle_to_expr(-utils::angles::pi_quarter));
            return std::list<ast::ptr<ast::Gate>>();
        } else {
            auto tmp = flush<ast::Gate>();
            tmp.emplace_back(ast::ptr<ast::Gate>(gate.clone()));
            return std::move(tmp);
        }
    }

    /* Declarations */
    void visit(ast::GateDecl& decl) override {
        // Initialize a new local state

        std::unordered_map<ast::VarAccess, int> local_map;
        std::unordered_map<int, ast::VarAccess> local_qubit;
        std::list<synthesis::phase_term> local_phases;
        synthesis::linear_op<bool> local_permutation;

        std::swap(qubit_map_, local_map);
        std::swap(map_qubit_, local_qubit);
        std::swap(phases_, local_phases);
        std::swap(permutation_, local_permutation);

        for (auto& var : decl.q_params())
            get_index(ast::VarAccess(decl.pos(), var));

        Replacer::visit(decl);

        // Flush remaining state
        for (auto& gate : flush<ast::Gate>())
            decl.body().emplace_back(std::move(gate));

        // Reset the state
        std::swap(qubit_map_, local_map);
        std::swap(map_qubit_, local_qubit);
        std::swap(phases_, local_phases);
        std::swap(permutation_, local_permutation);
    }

    void visit(ast::OracleDecl&) override {}
    void visit(ast::RegisterDecl& decl) override {
        if (decl.is_quantum()) {
            for (int i = 0; i < decl.size(); i++)
                get_index(ast::VarAccess(decl.pos(), decl.id(), i));
        }
    }
    void visit(ast::AncillaDecl& decl) override {
        for (int i = 0; i < decl.size(); i++)
            get_index(ast::VarAccess(decl.pos(), decl.id(), i));

        // TODO: make use of zero-valued ancillas
    }

    /* Program */
    void visit(ast::Program& prog) override {
        Replacer::visit(prog);

        // Synthesize the last leg
        for (auto& stmt : flush<ast::Stmt>())
            prog.body().emplace_back(std::move(stmt));
    }

  private:
    config config_;

    /* Algorithm state */
    std::unordered_map<ast::VarAccess, int> qubit_map_;
    std::unordered_map<int, ast::VarAccess> map_qubit_;
    std::list<synthesis::phase_term> phases_;
    synthesis::linear_op<bool> permutation_;

    void reset() {
        qubit_map_.clear();
        map_qubit_.clear();
        phases_.clear();
        permutation_.clear();
    }

    void add_phase(std::vector<bool> parity, ast::ptr<ast::Expr> e) {
        for (auto it = phases_.begin(); it != phases_.end(); it++) {
            if (it->first == parity) {
                parser::Position pos;
                it->second =
                    ast::BExpr::create(pos, std::move(it->second),
                                       ast::BinaryOp::Plus, std::move(e));
                return;
            }
        }

        phases_.push_back(std::make_pair(parity, std::move(e)));
    }

    // Flushes a cnot-dihedral operator (i.e. phases + permutation) to the
    // circuit before the given node
    template <typename T>
    std::list<ast::ptr<T>> flush() {
        std::list<ast::ptr<T>> ret;
        parser::Position pos;

        // Synthesize circuit
        for (auto& gate : synthesis::gray_synth(phases_, permutation_)) {
            std::visit(
                utils::overloaded{
                    [&ret, this](std::pair<int, int>& cx) {
                        ret.emplace_back(generate_cnot(cx.first, cx.second));
                    },
                    [&ret, this](std::pair<ast::ptr<ast::Expr>, int>& rz) {
                        ret.emplace_back(
                            generate_rz(std::move(rz.first), rz.second));
                    }},
                gate);
        }

        // Reset the cnot-dihedral circuit
        phases_.clear();
        for (std::size_t i = 0; i < qubit_map_.size(); i++) {
            for (std::size_t j = 0; j < qubit_map_.size(); j++) {
                permutation_[i][j] = i == j ? true : false;
            }
        }

        return ret;
    }

    bool is_zero(ast::Expr& expr) {
        auto val = expr.constant_eval();
        return val && (*val == 0);
    }

    int get_index(const ast::VarAccess& va) {
        if (qubit_map_.find(va) != qubit_map_.end())
            return qubit_map_[va];
        else {
            auto n = qubit_map_.size();
            qubit_map_[va] = static_cast<int>(n);
            map_qubit_.emplace(static_cast<int>(n), va);

            // Extend the current permutation
            permutation_.emplace_back(std::vector<bool>(n + 1, false));
            for (std::size_t i = 0; i < n; i++) {
                permutation_[i].emplace_back(false);
            }
            permutation_[n][n] = true;

            // Extend all other vectors
            for (auto& [vec, angle] : phases_)
                vec.emplace_back(false);

            return static_cast<int>(n);
        }
    }

    /* Gate generation */
    // Assumes basic gates (x, y, z, s, sdg, t, tdg, rz) are defined
    ast::ptr<ast::DeclaredGate> generate_rz(ast::ptr<ast::Expr> theta, int i) {
        auto c = theta->constant_eval();

        parser::Position pos;

        std::string name;
        std::vector<ast::ptr<ast::Expr>> cargs;
        std::vector<ast::VarAccess> qargs{(map_qubit_.find(i))->second};

        // Determine the name & classical arguments
        if (!c) {
            // Angle is conditional
            name = "rz";
            cargs.emplace_back(std::move(theta));
        } else {
            // Angle is constant
            if (*c == utils::pi) {
                // Z gate
                name = "z";
            } else if (*c == utils::pi / 2) {
                // S gate
                name = "s";
            } else if (*c == 3 * utils::pi / 2) {
                // Sdg gate
                name = "sdg";
            } else if (*c == utils::pi / 4) {
                // T gate
                name = "t";
            } else if (*c == 7 * utils::pi / 4) {
                // Tdg gate
                name = "tdg";
            } else {
                // Rz gate
                name = "rz";
                cargs.emplace_back(std::move(theta));
            }
        }

        return std::make_unique<ast::DeclaredGate>(pos, name, std::move(cargs),
                                                   std::move(qargs));
    }

    ast::ptr<ast::DeclaredGate> generate_cnot(int i, int j) {
        parser::Position pos;
        std::string name = "cx";
        std::vector<ast::ptr<ast::Expr>> cargs;
        std::vector<ast::VarAccess> qargs{(map_qubit_.find(i))->second,
                                          (map_qubit_.find(j))->second};

        return std::make_unique<ast::DeclaredGate>(pos, name, std::move(cargs),
                                                   std::move(qargs));
    }
};

/** \brief Performs CNOT optimization */
static void optimize_CNOT(ast::ASTNode& node) {
    CNOTOptimizer optimizer;
    optimizer.run(node);
}

/** \brief Performs CNOT optimization with configuration */
static void optimize_CNOT(ast::ASTNode& node,
                          const CNOTOptimizer::config& params) {
    CNOTOptimizer optimizer(params);
    optimizer.run(node);
}

} // namespace optimization
} // namespace staq
