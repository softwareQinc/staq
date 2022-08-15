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
 * \file output/cirq.hpp
 * \brief Cirq outputter
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
namespace ast = qasmtools::ast;

/* \brief Inliner overrides for lattice surgery */
static const std::set<std::string_view> ls_inline_overrides{
    "u3", "u2",  "u1", "cx",  "id", "u0", "x",  "y",  "z", "h",
    "s",  "sdg", "t",  "tdg", "rx", "ry", "rz", "cz", "cy"};

/**
 * \class staq::output::PauliOpCircuitCompiler
 * \brief Visitor for converting a QASM AST into Pauli Op circuit
 */
class PauliOpCircuitCompiler final : public ast::Visitor {
  public:
    json run(ast::Program& prog) {
        transformations::desugar(prog);
        transformations::inline_ast(prog, {false, ls_inline_overrides, "anc"});
        result_.clear();
        result_["layers"];
        ids_.clear();
        num_qubits_ = 0;
        prog.accept(*this);
        return std::move(result_);
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
        add_layer({stmt.q_arg()}, {"Z"}, "M");
    }

    void visit(ast::ResetStmt& stmt) {
        throw std::logic_error("Qubit reset not supported");
    }

    void visit(ast::IfStmt& stmt) {
        throw std::logic_error("Classical control not supported");
    }

    // Gates
    void visit(ast::UGate& gate) {
        throw std::logic_error("Universal unitary not supported");
    }

    void visit(ast::CNOTGate& gate) {
        throw std::logic_error("Built-in CX not supported (use cx instead)");
    }

    void visit(ast::BarrierGate&) {}

    void visit(ast::DeclaredGate& gate) {
        if (gate.name() == "u3") {
            auto phase1 = get_phase(gate.carg(2));
            auto phase2 = qasmtools::utils::Angle(1, 2);
            auto phase3 =
                get_phase(gate.carg(0)) + qasmtools::utils::Angle(1, 1);
            auto phase4 = qasmtools::utils::Angle(1, 2);
            auto phase5 =
                get_phase(gate.carg(1)) + qasmtools::utils::Angle(3, 1);
            add_layer(gate.qargs(), {"Z"}, to_phase_string(phase1 / 2));
            add_layer(gate.qargs(), {"X"}, to_phase_string(phase2 / 2));
            add_layer(gate.qargs(), {"Z"}, to_phase_string(phase3 / 2));
            add_layer(gate.qargs(), {"X"}, to_phase_string(phase4 / 2));
            add_layer(gate.qargs(), {"Z"}, to_phase_string(phase5 / 2));
        } else if (gate.name() == "u2") {
            auto phase1 =
                get_phase(gate.carg(1)) - qasmtools::utils::Angle(1, 2);
            auto phase2 = qasmtools::utils::Angle(1, 2);
            auto phase3 =
                get_phase(gate.carg(0)) + qasmtools::utils::Angle(1, 2);
            add_layer(gate.qargs(), {"Z"}, to_phase_string(phase1 / 2));
            add_layer(gate.qargs(), {"X"}, to_phase_string(phase2 / 2));
            add_layer(gate.qargs(), {"Z"}, to_phase_string(phase3 / 2));
        } else if (gate.name() == "u1" || gate.name() == "rz") {
            auto phase = get_phase(gate.carg(0));
            add_layer(gate.qargs(), {"Z"}, to_phase_string(phase / 2));
        } else if (gate.name() == "cx") {
            add_layer(gate.qargs(), {"Z", "X"}, "1/4");
            add_layer(gate.qargs(), {"Z", "I"}, "-1/4");
            add_layer(gate.qargs(), {"I", "X"}, "-1/4");
        } else if (gate.name() == "id" || gate.name() == "u0") {
        } else if (gate.name() == "x") {
            add_layer(gate.qargs(), {"X"}, "1/2");
        } else if (gate.name() == "z") {
            add_layer(gate.qargs(), {"Z"}, "1/2");
        } else if (gate.name() == "h") {
            add_layer(gate.qargs(), {"Z"}, "1/4");
            add_layer(gate.qargs(), {"X"}, "1/4");
            add_layer(gate.qargs(), {"Z"}, "1/4");
        } else if (gate.name() == "s") {
            add_layer(gate.qargs(), {"Z"}, "1/4");
        } else if (gate.name() == "sdg") {
            add_layer(gate.qargs(), {"Z"}, "-1/4");
        } else if (gate.name() == "t") {
            add_layer(gate.qargs(), {"Z"}, "1/8");
        } else if (gate.name() == "tdg") {
            add_layer(gate.qargs(), {"Z"}, "-1/8");
        } else if (gate.name() == "rx") {
            auto phase = get_phase(gate.carg(0));
            add_layer(gate.qargs(), {"X"}, to_phase_string(phase / 2));
        } else if (gate.name() == "cz") {
            add_layer(gate.qargs(), {"Z", "Z"}, "1/4");
            add_layer(gate.qargs(), {"Z", "I"}, "-1/4");
            add_layer(gate.qargs(), {"I", "Z"}, "-1/4");
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
        // Program body
        prog.foreach_stmt([this](auto& stmt) {
            if (typeid(stmt) != typeid(ast::GateDecl) &&
                typeid(stmt) != typeid(ast::RegisterDecl))
                stmt.accept(*this);
        });
    }

  private:
    json result_{};
    std::unordered_map<std::string, int> ids_{};
    int num_qubits_ = 0;

    int get_id(const ast::VarAccess& va) {
        return ids_[va.var()] + (*va.offset());
    }

    void add_layer(const std::vector<ast::VarAccess>& vas,
                   const std::vector<std::string>& ops,
                   const std::string& phase) {
        json layer;
        for (int i = 0; i < num_qubits_; i++) {
            layer["q" + std::to_string(i)] = "I";
        }
        for (int i = 0; i < vas.size(); i++) {
            layer["q" + std::to_string(get_id(vas[i]))] = ops[i];
        }
        layer["pi*"] = phase;
        result_["layers"].push_back(std::move(layer));
    }

    /* Evaluate expr as multiple of pi */
    static qasmtools::utils::Angle get_phase(ast::Expr& expr) {
        auto val = expr.constant_eval();
        if (val) {
            double phase_times_4 = (*val * 4.0) / qasmtools::utils::pi;
            if (lrint(phase_times_4) == phase_times_4) {
                return qasmtools::utils::Angle(lrint(phase_times_4), 4);
            } else {
                return qasmtools::utils::Angle(*val);
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
    auto output = PauliOpCircuitCompiler().run(prog);
    std::cout << output.dump(2) << "\n";
}

/** \brief Compiles an AST into lattice surgery instructions to a given output
 * stream */
void write_lattice_surgery(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
        std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
        auto output = PauliOpCircuitCompiler().run(prog);
        ofs << output.dump(2) << "\n";
    }

    ofs.close();
}

} // namespace output
} // namespace staq
