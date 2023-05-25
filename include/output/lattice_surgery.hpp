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
 * \file output/lattice_surgery.hpp
 * \brief Lattice surgery compiler
 */

#ifndef OUTPUT_LATTICESURGERY_HPP_
#define OUTPUT_LATTICESURGERY_HPP_

#include "qasmtools/ast/ast.hpp"
#include "qasmtools/utils/angle.hpp"
#include "transformations/desugar.hpp"
#include "transformations/inline.hpp"

#include <algorithm>
#include <complex>
#include <map>
#include <set>
#include <typeinfo>
#include <nlohmann/json.hpp>

namespace staq::output {

using json = nlohmann::json;
using Angle = qasmtools::utils::Angle;
namespace ast = qasmtools::ast;

/* \brief Inliner overrides for lattice surgery */
static const std::set<std::string_view> ls_inline_overrides{
    "u3", "u2",  "u1", "cx",  "id", "u0", "x",  "y",  "z", "h",
    "s",  "sdg", "t",  "tdg", "rx", "ry", "rz", "cz", "cy"};

/**
 * \brief Pauli rotations
 */
enum class PauliOperator : char { I = 'I', X = 'X', Y = 'Y', Z = 'Z' };

/**
 * \brief Anti-commute multiplication table
 */
static const std::map<std::pair<PauliOperator, PauliOperator>,
                      std::pair<std::complex<double>, PauliOperator>>
    PauliOperator_anticommute_tbl{
        {{PauliOperator::Z, PauliOperator::X},
         {std::complex<double>(0, 1), PauliOperator::Y}},
        {{PauliOperator::X, PauliOperator::Z},
         {std::complex<double>(0, -1), PauliOperator::Y}},
        {{PauliOperator::Y, PauliOperator::Z},
         {std::complex<double>(0, 1), PauliOperator::X}},
        {{PauliOperator::Z, PauliOperator::Y},
         {std::complex<double>(0, -1), PauliOperator::X}},
        {{PauliOperator::X, PauliOperator::Y},
         {std::complex<double>(0, 1), PauliOperator::Z}},
        {{PauliOperator::Y, PauliOperator::X},
         {std::complex<double>(0, -1), PauliOperator::Z}}};

class LayeredPauliOpCircuit;

/**
 * \class staq::output::PauliOpCircuit
 * \brief Representation of Pauli Op circuits
 * This class is mostly a translation from here:
 * https://github.com/latticesurgery-com/lattice-surgery-compiler/blob/dev/src/lsqecc/pauli_rotations/circuit.py#L30
 */
class PauliOpCircuit {
  public:
    using Op = std::pair<std::vector<PauliOperator>, std::string>;

    explicit PauliOpCircuit(int no_of_qubit) : qubit_num_(no_of_qubit) {}

    void add_pauli_block(Op op) { // add operation to end of circuit
        if (op.first.size() != qubit_num_) {
            throw std::logic_error("len(ops_list) != number of qubits");
        }
        ops_.push_back(std::move(op));
    }

    json to_json() const { // get circuit in json format
        json result;
        result["n"] = qubit_num_;
        result["layers"];
        for (auto& op : ops_) {
            json layer;
            for (int i = 0; i < qubit_num_; i++) {
                auto op_name = static_cast<char>(op.first[i]);
                if (op_name == 'I')
                    continue;
                layer["q" + std::to_string(i)] = std::string(1, op_name);
            }
            layer["pi*"] = op.second;
            result["layers"].push_back(std::move(layer));
        }
        return result;
    }

    // y-free equivalent circuit
    PauliOpCircuit to_y_free_equivalent() const {
        PauliOpCircuit ans{qubit_num_};
        for (auto& block : ops_) {
            auto y_free = get_y_free_equivalent(block);
            ans.ops_.splice(ans.ops_.end(), y_free);
        }
        return ans;
    }

    // TODO optimize this
    // push pi/4 rotations to end of circuit
    void litinski_transform() {
        decompose();
        std::vector<std::list<Op>::iterator> pushed_rotations;
        bool circuit_has_measurements = false;

        for (auto it = ops_.begin(); it != ops_.end(); ++it) {
            if (it->second == "M" || it->second == "-M") {
                circuit_has_measurements = true;
            } else if (it->second == "1/4" || it->second == "-1/4" ||
                       it->second == "1/2" || it->second == "-1/2") {
                pushed_rotations.push_back(it);
            }
        }

        for (auto it = pushed_rotations.rbegin(); it != pushed_rotations.rend();
             ++it) {
            auto index = *it;
            // TODO optimize this
            while (index != ops_.end()) {
                auto next_block = index;
                ++next_block;
                if (next_block == ops_.end()) {
                    break;
                }
                swap_adjacent_blocks(index); // TODO optimize this
                ++index;
            }
            if (circuit_has_measurements)
                ops_.pop_back();
        }
    }

    static bool are_commuting(const Op& block1, const Op& block2) {
        if (block1.first.size() != block2.first.size()) {
            throw std::logic_error("Blocks must have same number of qubits");
        }
        bool ret_val = true;
        for (int i = 0; i < block1.first.size(); i++) {
            if (!are_commuting(block1.first[i], block2.first[i])) {
                ret_val = !ret_val;
            }
        }
        return ret_val;
    }

    static bool are_commuting(PauliOperator a, PauliOperator b) {
        if (PauliOperator_anticommute_tbl.find({a, b}) ==
            PauliOperator_anticommute_tbl.end()) {
            return true;
        }
        return false;
    }

    static std::pair<std::complex<double>, PauliOperator>
    multiply_operators(PauliOperator a, PauliOperator b) {
        auto it = PauliOperator_anticommute_tbl.find({a, b});
        if (it != PauliOperator_anticommute_tbl.end()) {
            return it->second;
        }
        if (a == b) {
            return {1, PauliOperator::I};
        }
        if (a == PauliOperator::I || b == PauliOperator::I) {
            return {1, a == PauliOperator::I ? b : a};
        }
        throw std::logic_error("Uncaught multiply case");
    }

    friend class LayeredPauliOpCircuit;

  protected:
    int qubit_num_;
    std::list<Op> ops_;

  private:
    static std::list<Op> get_y_free_equivalent(const Op& block) {
        std::list<int> y_op_indices;
        Op y_free_block = block;
        for (int i = 0; i < block.first.size(); i++) {
            if (block.first[i] == PauliOperator::Y) {
                y_op_indices.push_back(i);
                y_free_block.first[i] = PauliOperator::X;
            }
        }

        if (y_op_indices.empty())
            return {std::move(y_free_block)};

        std::list<Op> left_rotations, right_rotations;
        if (y_op_indices.size() % 2 == 0) {
            int first_y_operator = y_op_indices.front();
            y_op_indices.pop_front();
            std::vector<PauliOperator> new_rotation_ops(block.first.size(),
                                                        PauliOperator::I);
            new_rotation_ops[first_y_operator] = PauliOperator::Z;
            left_rotations.emplace_back(new_rotation_ops, "1/4");
            right_rotations.emplace_back(new_rotation_ops, "-1/4");
        }

        std::vector<PauliOperator> new_rotation_ops(block.first.size(),
                                                    PauliOperator::I);
        for (int i : y_op_indices) {
            new_rotation_ops[i] = PauliOperator::Z;
        }

        left_rotations.emplace_back(new_rotation_ops, "1/4");
        right_rotations.emplace_back(new_rotation_ops, "-1/4");
        // return left_rotations + [y_free_block] + right_rotations
        left_rotations.push_back(std::move(y_free_block));
        left_rotations.splice(left_rotations.end(), right_rotations);
        return left_rotations;
    }

    void swap_adjacent_blocks(std::list<Op>::iterator index) {
        auto next_block = index;
        ++next_block;
        if (are_commuting(*index, *next_block)) {
            swap_adjacent_commuting_blocks(index);
        } else {
            swap_adjacent_anticommuting_blocks(index);
        }
    }

    static void swap_adjacent_commuting_blocks(std::list<Op>::iterator index) {
        auto next_block = index;
        ++next_block;
        std::iter_swap(index, next_block);
    }

    void
    swap_adjacent_anticommuting_blocks(std::list<Op>::iterator index) const {
        auto next_block = index;
        ++next_block;

        if (index->second == "1/4" || index->second == "-1/4") {
            std::complex<double> product_of_coefficients(1, 0);

            for (int i = 0; i < qubit_num_; i++) {
                auto [coeff, op] =
                    multiply_operators(index->first[i], next_block->first[i]);
                next_block->first[i] = op;
                product_of_coefficients *= coeff;
            }
            product_of_coefficients *= std::complex<double>(0, 1);
            if (next_block->second == "M" || next_block->second == "-M") {
                if (product_of_coefficients.real() < 0) { // flip phase
                    next_block->second = next_block->second == "M" ? "-M" : "M";
                }
            } else {
                if (product_of_coefficients.real() < 0) { // flip phase
                    if (next_block->second.front() == '-') {
                        next_block->second = next_block->second.substr(1);
                    } else {
                        next_block->second = "-" + next_block->second;
                    }
                }
            }
            std::iter_swap(index, next_block);
        } else if (index->second == "1/2" || index->second == "-1/2") {
            if (next_block->second == "M" || next_block->second == "-M") {
                next_block->second = next_block->second == "M" ? "-M" : "M";
            } else {
                if (next_block->second.front() == '-') {
                    next_block->second = next_block->second.substr(1);
                } else {
                    next_block->second = "-" + next_block->second;
                }
            }
            std::iter_swap(index, next_block);
        } else {
            throw std::logic_error("Can only swap pi/2 or pi/4 rotations");
        }
    }

    void decompose() { // decompose into { pi/2, pi/4, pi/8 } wherever possible
        std::list<Op> result;
        for (const auto& op : ops_) {
            for (const auto& p : decompose_phase(op.second)) {
                result.emplace_back(op.first, p);
            }
        }
        ops_.swap(result);
    }

    static std::vector<std::string> decompose_phase(const std::string& phase) {
        // since utils::Angle is in [0,2pi), Angle/2 will be in [0,pi)
        // we care about pi times { 0/1, 1/8, 1/4, 3/8, 1/2, 5/8, 3/4, 7/8 }
        if (phase == "0/1") {
            return {}; // identity
        } else if (phase == "3/8") {
            return {"1/4", "1/8"};
        } else if (phase == "5/8") {
            return {"1/2", "1/8"};
        } else if (phase == "3/4") {
            return {"1/2", "1/4"};
        } else if (phase == "7/8") {
            return {"1/2", "1/4", "1/8"};
        } else {
            return {phase}; // leave as-is
        }
    }
};

/**
 * \class staq::output::LayeredPauliOpCircuit
 * \brief Representation used for T count/depth and related optimizations
 */
class LayeredPauliOpCircuit {
    using Op = PauliOpCircuit::Op;
    int qubit_num_;
    std::list<std::list<Op>> layers_;
    std::list<Op> final_;

  public:
    explicit LayeredPauliOpCircuit(const PauliOpCircuit& c)
        : qubit_num_(c.qubit_num_) {
        bool expect_no_more_Ts = false; // pi/8 rotations come before all else
        for (auto const& op : c.ops_) {
            if (op.second == "1/8" || op.second == "-1/8") {
                if (expect_no_more_Ts) {
                    throw std::logic_error("pi/8 rotations must come before all "
                                           "pi/4 rotations and measurements");
                } else {
                    layers_.push_back({op});
                }
            } else if (op.second == "1/4" || op.second == "-1/4" ||
                       op.second == "1/2" || op.second == "-1/2" ||
                       op.second == "M" || op.second == "-M") {
                expect_no_more_Ts = true;
                final_.push_back(op);
            } else {
                throw std::logic_error("Unsupported phase: " + op.second);
            }
        }
    }

    // get circuit in json format, with T layers grouped
    json to_json() const {
        json result;
        result["n"] = qubit_num_;
        int t_count = 0;
        for (auto& layer : layers_) {
            t_count += layer.size();
        }
        result["T count"] = t_count;
        result["T depth"] = layers_.size();

        result["T layers"];
        for (auto& layer : layers_) {
            json layer_json;
            for (auto& op : layer) {
                json op_json;
                for (int i = 0; i < qubit_num_; i++) {
                    auto op_name = static_cast<char>(op.first[i]);
                    if (op_name == 'I')
                        continue;
                    op_json["q" + std::to_string(i)] = std::string(1, op_name);
                }
                op_json["pi*"] = op.second;
                layer_json.push_back(std::move(op_json));
            }
            result["T layers"].push_back(std::move(layer_json));
        }

        result["pi/4 rotations and measurements"];
        for (auto& op : final_) {
            json op_json;
            for (int i = 0; i < qubit_num_; i++) {
                auto op_name = static_cast<char>(op.first[i]);
                if (op_name == 'I')
                    continue;
                op_json["q" + std::to_string(i)] = std::string(1, op_name);
            }
            op_json["pi*"] = op.second;
            result["pi/4 rotations and measurements"].push_back(
                std::move(op_json));
        }

        return result;
    }

    // TODO optimize this
    /* greedy algorithm from page 6 of https://arxiv.org/pdf/1808.02892.pdf */
    void reduce() {
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto it = layers_.begin(); it != layers_.end();) {
                auto next_it = it;
                ++next_it;
                if (next_it == layers_.end()) {
                    break;
                }
                for (auto op = next_it->begin(); op != next_it->end();) {
                    bool commutes = true;
                    for (auto& op2 : *it) {
                        if (!PauliOpCircuit::are_commuting(*op, op2)) {
                            commutes = false;
                            break;
                        }
                    }
                    if (commutes) {
                        auto next_op = op;
                        ++next_op;
                        it->splice(it->end(), *next_it, op);
                        changed = true;
                        op = next_op;
                    } else {
                        ++op;
                    }
                }
                if (next_it->empty()) {
                    layers_.erase(next_it);
                } else {
                    it = next_it;
                }
            }
        }
    }
};

/**
 * \class staq::output::PauliOpCircuitCompiler
 * \brief Visitor for converting a QASM AST into Pauli Op circuit
 */
class PauliOpCircuitCompiler final : public ast::Visitor {
    bool skip_clifford_{false};

  public:
    PauliOpCircuitCompiler(bool skip_clifford = false)
        : skip_clifford_{skip_clifford} {}
    PauliOpCircuit run(ast::Program& prog) {
        transformations::desugar(prog);
        transformations::inline_ast(prog, {false, ls_inline_overrides, "anc"});
        ids_.clear();
        num_qubits_ = 0;
        prog.accept(*this);
        return circuit_;
    }

    // Variables
    void visit(ast::VarAccess&) override {}

    // Expressions
    void visit(ast::BExpr&) override {}
    void visit(ast::UExpr&) override {}
    void visit(ast::PiExpr&) override {}
    void visit(ast::IntExpr&) override {}
    void visit(ast::RealExpr&) override {}
    void visit(ast::VarExpr&) override {}

    // Statements
    void visit(ast::MeasureStmt& stmt) override {
        add_layer({stmt.q_arg()}, {PauliOperator::Z}, "M");
    }

    void visit(ast::ResetStmt& stmt) override {
        throw std::logic_error("Qubit reset not supported");
    }

    void visit(ast::IfStmt& stmt) override {
        throw std::logic_error("Classical control not supported");
    }

    // Gates
    void visit(ast::UGate& gate) override {
        std::vector<ast::VarAccess> qargs{gate.arg()};
        auto phase1 = get_phase(gate.lambda());
        auto phase2 = get_phase(gate.theta());
        auto phase3 = get_phase(gate.phi());
        add_layer(qargs, {PauliOperator::Z}, to_phase_string(phase1 / 2));
        add_layer(qargs, {PauliOperator::Y}, to_phase_string(phase2 / 2));
        add_layer(qargs, {PauliOperator::Z}, to_phase_string(phase3 / 2));
    }

    void visit(ast::CNOTGate& gate) override {
        if (skip_clifford_)
            return;
        std::vector<ast::VarAccess> qargs{gate.ctrl(), gate.tgt()};
        add_layer(qargs, {PauliOperator::Z, PauliOperator::X}, "1/4");
        add_layer(qargs, {PauliOperator::Z, PauliOperator::I}, "-1/4");
        add_layer(qargs, {PauliOperator::I, PauliOperator::X}, "-1/4");
    }

    void visit(ast::BarrierGate&) override {}

    void visit(ast::DeclaredGate& gate) override {
        if (gate.name() == "u3") {
            auto phase1 = get_phase(gate.carg(2));
            auto phase2 = Angle(1, 2);
            auto phase3 = get_phase(gate.carg(0)) + Angle(1, 1);
            auto phase4 = Angle(1, 2);
            auto phase5 = get_phase(gate.carg(1)) + Angle(3, 1);
            add_layer(gate.qargs(), {PauliOperator::Z},
                      to_phase_string(phase1 / 2));
            add_layer(gate.qargs(), {PauliOperator::X},
                      to_phase_string(phase2 / 2));
            add_layer(gate.qargs(), {PauliOperator::Z},
                      to_phase_string(phase3 / 2));
            add_layer(gate.qargs(), {PauliOperator::X},
                      to_phase_string(phase4 / 2));
            add_layer(gate.qargs(), {PauliOperator::Z},
                      to_phase_string(phase5 / 2));
        } else if (gate.name() == "u2") {
            auto phase1 = get_phase(gate.carg(1)) - Angle(1, 2);
            auto phase2 = Angle(1, 2);
            auto phase3 = get_phase(gate.carg(0)) + Angle(1, 2);
            add_layer(gate.qargs(), {PauliOperator::Z},
                      to_phase_string(phase1 / 2));
            add_layer(gate.qargs(), {PauliOperator::X},
                      to_phase_string(phase2 / 2));
            add_layer(gate.qargs(), {PauliOperator::Z},
                      to_phase_string(phase3 / 2));
        } else if (gate.name() == "u1" || gate.name() == "rz") {
            auto phase = get_phase(gate.carg(0));
            add_layer(gate.qargs(), {PauliOperator::Z},
                      to_phase_string(phase / 2));
        } else if (gate.name() == "cx") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::X},
                          "1/4");
                add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::I},
                          "-1/4");
                add_layer(gate.qargs(), {PauliOperator::I, PauliOperator::X},
                          "-1/4");
            }
        } else if (gate.name() == "id" || gate.name() == "u0") {
        } else if (gate.name() == "x") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::X}, "1/2");
            }
        } else if (gate.name() == "y") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Y}, "1/2");
            }
        } else if (gate.name() == "z") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Z}, "1/2");
            }
        } else if (gate.name() == "h") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Z}, "1/4");
                add_layer(gate.qargs(), {PauliOperator::X}, "1/4");
                add_layer(gate.qargs(), {PauliOperator::Z}, "1/4");
            }
        } else if (gate.name() == "s") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Z}, "1/4");
            }
        } else if (gate.name() == "sdg") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Z}, "-1/4");
            }
        } else if (gate.name() == "t") {
            add_layer(gate.qargs(), {PauliOperator::Z}, "1/8");
        } else if (gate.name() == "tdg") {
            add_layer(gate.qargs(), {PauliOperator::Z}, "-1/8");
        } else if (gate.name() == "rx") {
            auto phase = get_phase(gate.carg(0));
            add_layer(gate.qargs(), {PauliOperator::X},
                      to_phase_string(phase / 2));
        } else if (gate.name() == "ry") {
            auto phase = get_phase(gate.carg(0));
            add_layer(gate.qargs(), {PauliOperator::Y},
                      to_phase_string(phase / 2));
        } else if (gate.name() == "cz") {
            if (!skip_clifford_) {
                add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::Z},
                          "1/4");
                add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::I},
                          "-1/4");
                add_layer(gate.qargs(), {PauliOperator::I, PauliOperator::Z},
                          "-1/4");
            }
        } else if (gate.name() == "cy") {
            add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::Y},
                      "1/4");
            add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::I},
                      "-1/4");
            add_layer(gate.qargs(), {PauliOperator::I, PauliOperator::Y},
                      "-1/4");
        } else {
            throw std::logic_error("Unsupported gate name: " + gate.name());
        }
    }

    // Declarations
    void visit(ast::GateDecl& decl) override {}

    void visit(ast::OracleDecl& decl) override {
        throw std::logic_error("Oracle declarations not supported");
    }

    void visit(ast::RegisterDecl& decl) override {
        if (decl.is_quantum()) {
            ids_[decl.id()] = num_qubits_;
            num_qubits_ += decl.size();
        }
    }

    void visit(ast::AncillaDecl& decl) override {
        throw std::logic_error("Local ancillas not supported");
    }

    // Program
    void visit(ast::Program& prog) override {
        // Gate & qubit declarations
        prog.foreach_stmt([this](auto& stmt) {
            if (typeid(stmt) == typeid(ast::GateDecl) ||
                typeid(stmt) == typeid(ast::RegisterDecl))
                stmt.accept(*this);
        });
        circuit_ = PauliOpCircuit(num_qubits_);
        // Program body
        prog.foreach_stmt([this](auto& stmt) {
            if (typeid(stmt) != typeid(ast::GateDecl) &&
                typeid(stmt) != typeid(ast::RegisterDecl))
                stmt.accept(*this);
        });
    }

  private:
    PauliOpCircuit circuit_{0};
    std::unordered_map<std::string, int> ids_{};
    int num_qubits_ = 0;

    int get_id(const ast::VarAccess& va) {
        return ids_[va.var()] + (*va.offset());
    }

    // TODO check this and remove I if possible
    void add_layer(const std::vector<ast::VarAccess>& vas,
                   const std::vector<PauliOperator>& ops,
                   const std::string& phase) {
        std::vector<PauliOperator> layer(num_qubits_, PauliOperator::I);
        for (int i = 0; i < vas.size(); i++) {
            layer[get_id(vas[i])] = ops[i];
        }
        circuit_.add_pauli_block({std::move(layer), phase});
    }

    /* Evaluate expr as multiple of pi */
    static Angle get_phase(ast::Expr& expr) {
        auto val = expr.constant_eval();
        if (val) {
            double phase_times_4 = (*val * 4.0) / qasmtools::utils::pi;
            if (static_cast<double>(lrint(phase_times_4)) == phase_times_4) {
                return {static_cast<int>(lrint(phase_times_4)), 4};
            } else {
                return {*val / qasmtools::utils::pi};
            }
        }
        throw std::logic_error("Could not evaluate expression");
    }

    static std::string to_phase_string(const qasmtools::utils::Angle& ang) {
        if (ang.is_symbolic()) {
            auto [a, b] = *ang.symbolic_value();
            return std::to_string(a) + "/" + std::to_string(b);
        } else {
            return std::to_string(ang.numeric_value());
        }
    }
};

/** \brief Compiles an AST into lattice surgery instructions to stdout */
void output_lattice_surgery(ast::Program& prog, bool skip_clifford = false,
                            bool skip_litinski = false,
                            bool skip_reduce = false,

                            std::ostream& os = std::cout) {
    // JSON parts
    const std::string FIRST{"1. Circuit as Pauli rotations"};
    const std::string SECOND{"2. Circuit after the Litinski Transform"};
    const std::string THIRD{"3. T-layered circuit"};

    json out;
    out[FIRST];
    auto circuit = PauliOpCircuitCompiler(skip_clifford).run(prog);
    out[FIRST] = circuit.to_json();

    out[SECOND];
    if (!skip_clifford && !skip_litinski) {
        circuit.litinski_transform();
        out[SECOND] = circuit.to_json();
    }

    out[THIRD];
    try {
        if (!skip_litinski || skip_clifford) {
            LayeredPauliOpCircuit lcircuit(circuit);
            if (!skip_reduce) {
                lcircuit.reduce();
            }
            out[THIRD] = lcircuit.to_json();
        }
    } catch (std::logic_error& err) {
        std::string err_msg(err.what());
        if (err_msg.find("Unsupported phase: ") != std::string::npos) {
            std::cerr << "Warning: Circuit is not in Clifford + T\n";
        } else
            throw;
    }
    std::cout << out.dump(2) << "\n";
}

/** \brief Compiles an AST into lattice surgery instructions to a given output
 * file */
void write_lattice_surgery(ast::Program& prog, const std::string& fname,
                           bool skip_clifford = false,
                           bool skip_litinski = false,
                           bool skip_reduce = false) {
    std::ofstream ofs{fname};

    if (!ofs.good()) {
        std::cerr << "Error: failed to open output file " << fname << "\n";
        exit(-1);
    }
    output_lattice_surgery(prog, skip_clifford, skip_litinski, skip_reduce,
                           ofs);
}

/** \brief Compiles an AST into lattice surgery instructions to a std::string
 * representing a json object
 */
std::string lattice_surgery(ast::Program& prog, bool skip_clifford = false,
                            bool skip_litinski = false,
                            bool skip_reduce = false) {
    std::stringstream ss;
    output_lattice_surgery(prog, skip_clifford, skip_litinski, skip_reduce, ss);
    return ss.str();
}

} /* namespace staq::output */

#endif /* OUTPUT_LATTICESURGERY_HPP_ */
