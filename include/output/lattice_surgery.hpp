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
 * \file output/lattice_surgery.hpp
 * \brief Lattice surgery compiler
 */

#pragma once

#include "qasmtools/ast/ast.hpp"
#include "qasmtools/utils/angle.hpp"
#include "transformations/desugar.hpp"
#include "transformations/inline.hpp"

#include <typeinfo>
#include <nlohmann/json.hpp>

namespace staq {
namespace output {

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
 * \class staq::output::PauliOpCircuit
 * \brief Representation of Pauli Op circuits
 */
class PauliOpCircuit {
  public:
    using Op = std::pair<std::vector<PauliOperator>, std::string>;

    explicit PauliOpCircuit(int no_of_qubit) : qubit_num_(no_of_qubit) {}
    void add_pauli_block(Op op) {
        if (op.first.size() != qubit_num_) {
            throw std::logic_error("len(ops_list) != number of qubits");
        }
        ops_.push_back(std::move(op));
    }
    json to_json() {
        json result;
        result["layers"];
        for (const auto& op : ops_) {
            json layer;
            for (int i = 0; i < qubit_num_; i++) {
                layer["q" + std::to_string(i)] =
                    std::string(1, static_cast<char>(op.first[i]));
            }
            layer["pi*"] = op.second;
            result["layers"].push_back(std::move(layer));
        }
        return result;
    }
    /**
     * \brief Copied from PauliOpCircuit.to_y_free_equivalent here:
     * https://github.com/latticesurgery-com/lattice-surgery-compiler/blob/dev/src/lsqecc/pauli_rotations/circuit.py#L135
     */
    PauliOpCircuit to_y_free_equivalent() {
        PauliOpCircuit ans{qubit_num_};
        for (const auto& block : ops_) {
            auto y_free = get_y_free_equivalent(block);
            ans.ops_.splice(ans.ops_.end(), y_free);
        }
        return ans;
    }

  private:
    int qubit_num_;
    std::list<Op> ops_;

    /**
     * \brief Copied from PauliProductOperation.to_y_free_equivalent here:
     * https://github.com/latticesurgery-com/lattice-surgery-compiler/blob/dev/src/lsqecc/pauli_rotations/rotation.py#L171
     */
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
            left_rotations.push_back({new_rotation_ops, "1/4"});
            right_rotations.push_back({new_rotation_ops, "-1/4"});
        }

        std::vector<PauliOperator> new_rotation_ops(block.first.size(),
                                                    PauliOperator::I);
        for (int i : y_op_indices) {
            new_rotation_ops[i] = PauliOperator::Z;
        }
        left_rotations.push_back({new_rotation_ops, "1/4"});
        right_rotations.push_back({new_rotation_ops, "-1/4"});
        // return left_rotations + [y_free_block] + right_rotations
        left_rotations.push_back(std::move(y_free_block));
        left_rotations.splice(left_rotations.end(), right_rotations);
        return left_rotations;
    }
};

/**
 * \class staq::output::PauliOpCircuitCompiler
 * \brief Visitor for converting a QASM AST into Pauli Op circuit
 */
class PauliOpCircuitCompiler final : public ast::Visitor {
  public:
    PauliOpCircuit run(ast::Program& prog) {
        transformations::desugar(prog);
        transformations::inline_ast(prog, {false, ls_inline_overrides, "anc"});
        ids_.clear();
        num_qubits_ = 0;
        prog.accept(*this);
        return circuit_;
    }

    // Variables
    void visit(ast::VarAccess&) {}

    // Expressions
    void visit(ast::BExpr&) {}
    void visit(ast::UExpr&) {}
    void visit(ast::PiExpr&) {}
    void visit(ast::IntExpr&) {}
    void visit(ast::RealExpr&) {}
    void visit(ast::VarExpr&) {}

    // Statements
    void visit(ast::MeasureStmt& stmt) {
        add_layer({stmt.q_arg()}, {PauliOperator::Z}, "M");
    }

    void visit(ast::ResetStmt& stmt) {
        throw std::logic_error("Qubit reset not supported");
    }

    void visit(ast::IfStmt& stmt) {
        throw std::logic_error("Classical control not supported");
    }

    // Gates
    void visit(ast::UGate& gate) {
        std::vector<ast::VarAccess> qargs{gate.arg()};
        auto phase1 = get_phase(gate.lambda());
        auto phase2 = get_phase(gate.theta());
        auto phase3 = get_phase(gate.phi());
        add_layer(qargs, {PauliOperator::Z}, to_phase_string(phase1 / 2));
        add_layer(qargs, {PauliOperator::Y}, to_phase_string(phase2 / 2));
        add_layer(qargs, {PauliOperator::Z}, to_phase_string(phase3 / 2));
    }

    void visit(ast::CNOTGate& gate) {
        std::vector<ast::VarAccess> qargs{gate.ctrl(), gate.tgt()};
        add_layer(qargs, {PauliOperator::Z, PauliOperator::X}, "1/4");
        add_layer(qargs, {PauliOperator::Z, PauliOperator::I}, "-1/4");
        add_layer(qargs, {PauliOperator::I, PauliOperator::X}, "-1/4");
    }

    void visit(ast::BarrierGate&) {}

    void visit(ast::DeclaredGate& gate) {
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
            add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::X},
                      "1/4");
            add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::I},
                      "-1/4");
            add_layer(gate.qargs(), {PauliOperator::I, PauliOperator::X},
                      "-1/4");
        } else if (gate.name() == "id" || gate.name() == "u0") {
        } else if (gate.name() == "x") {
            add_layer(gate.qargs(), {PauliOperator::X}, "1/2");
        } else if (gate.name() == "y") {
            add_layer(gate.qargs(), {PauliOperator::Y}, "1/2");
        } else if (gate.name() == "z") {
            add_layer(gate.qargs(), {PauliOperator::Z}, "1/2");
        } else if (gate.name() == "h") {
            add_layer(gate.qargs(), {PauliOperator::Z}, "1/4");
            add_layer(gate.qargs(), {PauliOperator::X}, "1/4");
            add_layer(gate.qargs(), {PauliOperator::Z}, "1/4");
        } else if (gate.name() == "s") {
            add_layer(gate.qargs(), {PauliOperator::Z}, "1/4");
        } else if (gate.name() == "sdg") {
            add_layer(gate.qargs(), {PauliOperator::Z}, "-1/4");
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
            add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::Z},
                      "1/4");
            add_layer(gate.qargs(), {PauliOperator::Z, PauliOperator::I},
                      "-1/4");
            add_layer(gate.qargs(), {PauliOperator::I, PauliOperator::Z},
                      "-1/4");
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
    void visit(ast::GateDecl& decl) {}

    void visit(ast::OracleDecl& decl) {
        throw std::logic_error("Oracle declarations not supported");
    }

    void visit(ast::RegisterDecl& decl) {
        if (decl.is_quantum()) {
            ids_[decl.id()] = num_qubits_;
            num_qubits_ += decl.size();
        }
    }

    void visit(ast::AncillaDecl& decl) {
        throw std::logic_error("Local ancillas not supported");
    }

    // Program
    void visit(ast::Program& prog) {
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
            if (lrint(phase_times_4) == phase_times_4) {
                return Angle(lrint(phase_times_4), 4);
            } else {
                return Angle(*val / qasmtools::utils::pi);
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

/** \brief Compiles an AST into lattice surgery instructions to a stdout */
void output_lattice_surgery(ast::Program& prog) {
    auto circuit = PauliOpCircuitCompiler().run(prog);
    std::cout << circuit.to_json().dump(2) << "\n";
}

/** \brief Compiles an AST into lattice surgery instructions to a given output
 * stream */
void write_lattice_surgery(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
        std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
        auto circuit = PauliOpCircuitCompiler().run(prog);
        ofs << circuit.to_json().dump(2) << "\n";
    }

    ofs.close();
}

} // namespace output
} // namespace staq
