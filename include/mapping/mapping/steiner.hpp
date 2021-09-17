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
 * \file mapping/mapping/steiner.hpp
 * \brief Steiner tree-based hardware mapping
 */

#pragma once

#include <qasm/ast/traversal.hpp>
#include "synthesis/linear_reversible.hpp"
#include "synthesis/cnot_dihedral.hpp"
#include "mapping/device.hpp"
#include <qasm/utils/templates.hpp>

#include <vector>

using namespace qasm;

namespace staq {
namespace mapping {

/**
 * \class staq::mapping::SteinerMapper
 * \brief Steiner tree based re-synthesizing mapper
 * \note Assumes the circuit has a single global register with the configured
 * name
 *
 * Re-synthesizes an entire circuit by breaking into cnot-dihedral "chunks"
 * and resynthesizing those using gray-synth (arXiv:1712.01859) extended
 * with a device dependent mapping technique based on Steiner trees
 * (arXiv:1904.01972)
 */
class SteinerMapper final : public ast::Replacer {
  public:
    struct config {
        std::string register_name = "q";
    };

    SteinerMapper(Device& device) : Replacer(), device_(device) {
        permutation_ = synthesis::linear_op<bool>(
            device.qubits_, std::vector<bool>(device.qubits_, false));
        for (auto i = 0; i < device.qubits_; i++) {
            permutation_[i][i] = true;
        }
    }

    // Ignore declarations if they were left in during inlining
    void visit(ast::GateDecl&) override {}
    void visit(ast::OracleDecl&) override {}

    void visit(ast::Program& prog) override {
        Replacer::visit(prog);

        // Synthesize the last leg
        for (auto& gate :
             synthesis::gray_steiner(phases_, permutation_, device_)) {
            std::visit(
                utils::overloaded{
                    [this, &prog](std::pair<int, int>& cx) {
                        if (device_.coupled(cx.first, cx.second)) {
                            prog.body().emplace_back(
                                generate_cnot(cx.first, cx.second, prog.pos()));
                        } else if (device_.coupled(cx.second, cx.first)) {
                            prog.body().emplace_back(
                                generate_hadamard(cx.first, prog.pos()));
                            prog.body().emplace_back(
                                generate_hadamard(cx.second, prog.pos()));
                            prog.body().emplace_back(
                                generate_cnot(cx.first, cx.second, prog.pos()));
                            prog.body().emplace_back(
                                generate_hadamard(cx.first, prog.pos()));
                            prog.body().emplace_back(
                                generate_hadamard(cx.second, prog.pos()));
                        } else {
                            throw std::logic_error(
                                "CNOT between non-coupled vertices!");
                        }
                    },
                    [this, &prog](std::pair<ast::ptr<ast::Expr>, int>& rz) {
                        prog.body().emplace_back(generate_rz(
                            std::move(rz.first), rz.second, prog.pos()));
                    }},
                gate);
        }
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::CNOTGate& gate) override {
        auto ctrl = get_index(gate.ctrl());
        auto tgt = get_index(gate.tgt());

        if (in_bounds(ctrl) && in_bounds(tgt)) {
            synthesis::operator^=(permutation_[tgt], permutation_[ctrl]);
        } else {
            throw std::logic_error("CNOT argument(s) out of device bounds!");
        }

        // Delete the gate
        return std::list<ast::ptr<ast::Gate>>();
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::UGate& gate) override {
        if (is_zero(gate.theta()) && is_zero(gate.phi())) {
            // It's a z-axis rotation
            auto angle = ast::ptr<ast::Expr>(gate.lambda().clone());
            auto idx = get_index(gate.arg());

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else {
            return flush<ast::Gate>(gate);
        }
    }
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {
        auto name = gate.name();

        if (name == "rz" || name == "u1") {
            auto angle = ast::ptr<ast::Expr>(gate.carg(0).clone());
            auto idx = get_index(gate.qarg(0));
            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "z") {
            auto angle = ast::angle_to_expr(utils::angles::pi);
            auto idx = get_index(gate.qarg(0));

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "s") {
            auto angle = ast::angle_to_expr(utils::angles::pi_half);
            auto idx = get_index(gate.qarg(0));

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "sdg") {
            auto angle = ast::angle_to_expr(-utils::angles::pi_half);
            auto idx = get_index(gate.qarg(0));

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "t") {
            auto angle = ast::angle_to_expr(utils::angles::pi_quarter);
            auto idx = get_index(gate.qarg(0));

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else if (name == "tdg") {
            auto angle = ast::angle_to_expr(-utils::angles::pi_quarter);
            auto idx = get_index(gate.qarg(0));

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], std::move(angle));
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

            return std::list<ast::ptr<ast::Gate>>();
        } else {
            return flush<ast::Gate>(gate);
        }
    }

    // Always generate a synthesis event
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::IfStmt& stmt) override {
        return flush<ast::Stmt>(stmt);
    }
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::BarrierGate& stmt) override {
        return flush<ast::Gate>(stmt);
    }
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::MeasureStmt& stmt) override {
        return flush<ast::Stmt>(stmt);
    }
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::ResetStmt& stmt) override {
        return flush<ast::Stmt>(stmt);
    }

  private:
    Device device_;
    config config_;

    // Accumulating data
    std::list<synthesis::phase_term> phases_;
    synthesis::linear_op<bool> permutation_;

    void add_phase(std::vector<bool> parity, ast::ptr<ast::Expr> angle) {
        for (auto it = phases_.begin(); it != phases_.end(); it++) {
            if (it->first == parity) {
                auto tmp = angle->pos();
                it->second =
                    ast::BExpr::create(tmp, std::move(it->second),
                                       ast::BinaryOp::Plus, std::move(angle));
                return;
            }
        }

        phases_.push_back(std::make_pair(parity, std::move(angle)));
    }

    // Flushes a cnot-dihedral operator (i.e. phases + permutation) to the
    // circuit before the given node
    template <typename T>
    std::list<ast::ptr<T>> flush(T& node) {
        std::list<ast::ptr<T>> ret;

        // Synthesize circuit
        for (auto& gate :
             synthesis::gray_steiner(phases_, permutation_, device_)) {
            std::visit(
                utils::overloaded{
                    [&ret, this, &node](std::pair<int, int>& cx) {
                        if (device_.coupled(cx.first, cx.second)) {
                            ret.emplace_back(
                                generate_cnot(cx.first, cx.second, node.pos()));
                        } else if (device_.coupled(cx.second, cx.first)) {
                            ret.emplace_back(
                                generate_hadamard(cx.first, node.pos()));
                            ret.emplace_back(
                                generate_hadamard(cx.second, node.pos()));
                            ret.emplace_back(
                                generate_cnot(cx.first, cx.second, node.pos()));
                            ret.emplace_back(
                                generate_hadamard(cx.first, node.pos()));
                            ret.emplace_back(
                                generate_hadamard(cx.second, node.pos()));
                        } else {
                            throw std::logic_error(
                                "CNOT between non-coupled vertices!");
                        }
                    },
                    [&ret, this,
                     &node](std::pair<ast::ptr<ast::Expr>, int>& rz) {
                        ret.emplace_back(generate_rz(std::move(rz.first),
                                                     rz.second, node.pos()));
                    }},
                gate);
        }
        ret.emplace_back(ast::ptr<T>(node.clone()));

        // Reset the cnot-dihedral circuit
        phases_.clear();
        for (auto i = 0; i < device_.qubits_; i++) {
            for (auto j = 0; j < device_.qubits_; j++) {
                permutation_[i][j] = i == j ? true : false;
            }
        }

        return ret;
    }

    bool in_bounds(int i) { return 0 <= i && i < device_.qubits_; }

    bool is_zero(ast::Expr& expr) {
        auto val = expr.constant_eval();
        return val && (*val == 0);
    }

    int get_index(ast::VarAccess& va) {
        if (va.offset())
            return *(va.offset());
        else
            throw std::logic_error(
                "Gate argument is not a register dereference!");
    }

    // Gate generation
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

    ast::ptr<ast::UGate> generate_rz(ast::ptr<ast::Expr> angle, int i,
                                     parser::Position pos) {
        auto tgt = ast::VarAccess(pos, config_.register_name, i);

        auto theta = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 0));
        auto phi = std::make_unique<ast::IntExpr>(ast::IntExpr(pos, 0));

        return std::make_unique<ast::UGate>(
            ast::UGate(pos, std::move(theta), std::move(phi), std::move(angle),
                       std::move(tgt)));
    }
};

/**
 * \class staq::mapping::SteinerDry
 * \brief Steiner mapper dry run
 * \note Utility class for optimizing over initial layouts
 *
 * Does a dry run of the Steiner mapping algorithm (i.e. doesn't actually
 * change the ast) with a particular layout and collects information about
 * the number of CNOT gates the synthesized circuit would use.
 */
class SteinerDry final : public ast::Traverse {
  public:
    SteinerDry(Device& device) : Traverse(), device_(device) {
        permutation_ = synthesis::linear_op<bool>(
            device.qubits_, std::vector<bool>(device.qubits_, false));
        for (auto i = 0; i < device.qubits_; i++) {
            permutation_[i][i] = true;
        }
    }

    int get_cnot_count(ast::Program& prog, const layout& l) {
        layout_ = l;
        cnots_ = 0;
        visit(prog);
        return cnots_;
    }

    void visit(ast::GateDecl&) override {}
    void visit(ast::OracleDecl&) override {}

    void visit(ast::Program& prog) override {
        Traverse::visit(prog);

        // Synthesize the last leg
        for (auto& gate :
             synthesis::gray_steiner(phases_, permutation_, device_)) {
            std::visit(
                utils::overloaded{
                    [this, &prog](std::pair<int, int>& cx) {
                        if (device_.coupled(cx.first, cx.second) ||
                            device_.coupled(cx.second, cx.first)) {
                            cnots_++;
                        } else {
                            throw std::logic_error(
                                "CNOT between non-coupled vertices!");
                        }
                    },
                    [this, &prog](std::pair<ast::ptr<ast::Expr>, int>& rz) {}},
                gate);
        }

        // Reset the cnot-dihedral circuit
        phases_.clear();
        for (auto i = 0; i < device_.qubits_; i++) {
            for (auto j = 0; j < device_.qubits_; j++) {
                permutation_[i][j] = i == j ? true : false;
            }
        }
    }

    void visit(ast::CNOTGate& gate) override {
        auto ctrl = layout_[gate.ctrl()];
        auto tgt = layout_[gate.tgt()];

        if (in_bounds(ctrl) && in_bounds(tgt)) {
            synthesis::operator^=(permutation_[tgt], permutation_[ctrl]);
        } else {
            throw std::logic_error("CNOT argument(s) out of device bounds!");
        }
    }

    void visit(ast::UGate& gate) override {
        if (is_zero(gate.theta()) && is_zero(gate.phi())) {
            // It's a z-axis rotation
            auto angle = nullptr;
            auto idx = layout_[gate.arg()];

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }

        } else {
            flush<ast::Gate>(gate);
        }
    }

    void visit(ast::DeclaredGate& gate) override {
        auto name = gate.name();

        if (name == "rz" || name == "u1") {
            auto angle = nullptr;
            auto idx = layout_[gate.qarg(0)];
            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }
        } else if (name == "z") {
            auto angle = nullptr;
            auto idx = layout_[gate.qarg(0)];

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }
        } else if (name == "s") {
            auto angle = nullptr;
            auto idx = layout_[gate.qarg(0)];

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }
        } else if (name == "sdg") {
            auto angle = nullptr;
            auto idx = layout_[gate.qarg(0)];

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }
        } else if (name == "t") {
            auto angle = nullptr;
            auto idx = layout_[gate.qarg(0)];

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }
        } else if (name == "tdg") {
            auto angle = nullptr;
            auto idx = layout_[gate.qarg(0)];

            if (in_bounds(idx)) {
                add_phase(permutation_[idx], angle);
            } else {
                throw std::logic_error(
                    "Unitary argument out of device bounds!");
            }
        } else {
            flush<ast::Gate>(gate);
        }
    }

    // Always generate a synthesis event
    void visit(ast::IfStmt& stmt) override { return flush<ast::Stmt>(stmt); }
    void visit(ast::BarrierGate& stmt) override {
        return flush<ast::Gate>(stmt);
    }
    void visit(ast::MeasureStmt& stmt) override {
        return flush<ast::Stmt>(stmt);
    }
    void visit(ast::ResetStmt& stmt) override { return flush<ast::Stmt>(stmt); }

  private:
    Device device_;
    layout layout_;
    int cnots_ = 0;

    // Accumulating data
    std::list<synthesis::phase_term> phases_;
    synthesis::linear_op<bool> permutation_;

    void add_phase(std::vector<bool> parity, ast::ptr<ast::Expr> angle) {
        for (auto it = phases_.begin(); it != phases_.end(); it++) {
            if (it->first == parity) {
                return;
            }
        }

        phases_.push_back(std::make_pair(parity, std::move(angle)));
    }

    // Flushes a cnot-dihedral operator (i.e. phases + permutation) to the
    // circuit before the given node
    template <typename T>
    void flush(T& node) {
        // Synthesize circuit
        for (auto& gate :
             synthesis::gray_steiner(phases_, permutation_, device_)) {
            std::visit(
                utils::overloaded{
                    [this, &node](std::pair<int, int>& cx) {
                        if (device_.coupled(cx.first, cx.second) ||
                            device_.coupled(cx.second, cx.first)) {
                            cnots_++;
                        } else {
                            throw std::logic_error(
                                "CNOT between non-coupled vertices!");
                        }
                    },
                    [this, &node](std::pair<ast::ptr<ast::Expr>, int>& rz) {}},
                gate);
        }

        // Reset the cnot-dihedral circuit
        phases_.clear();
        for (auto i = 0; i < device_.qubits_; i++) {
            for (auto j = 0; j < device_.qubits_; j++) {
                permutation_[i][j] = i == j ? true : false;
            }
        }
    }

    bool in_bounds(int i) { return 0 <= i && i < device_.qubits_; }

    bool is_zero(ast::Expr& expr) {
        auto val = expr.constant_eval();
        return val && (*val == 0);
    }
};

/**
 * \brief Layout optimization for the Steiner mapper via hill climb
 *
 * Repeatedly performs dry-runs, modifying the qubit mapping with a
 * single swap each time.
 */
void optimize_steiner_layout(Device& device, layout& init, ast::Program& prog) {
    SteinerDry alg(device);
    int current_min = alg.get_cnot_count(prog, init);

outer:
    for (auto it = init.begin(); it != init.end(); it++) {
        for (auto ti = std::next(it); ti != init.end(); ti++) {
            std::swap(it->second, ti->second);
            auto cnot_count = alg.get_cnot_count(prog, init);
            if (cnot_count < current_min) {
                current_min = cnot_count;
                goto outer;
            }
            std::swap(it->second, ti->second);
        }
    }
}

/** \brief Applies the Steiner mapper to an AST given a physical device */
void steiner_mapping(Device& device, ast::Program& prog) {
    SteinerMapper mapper(device);
    prog.accept(mapper);
}
} // namespace mapping
} // namespace staq
