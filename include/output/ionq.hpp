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
 * \file output/ionq.hpp
 * \brief IonQ outputter
 */

#ifndef OUTPUT_IONQ_HPP_
#define OUTPUT_IONQ_HPP_

#include <fstream>
#include <iomanip>
#include <typeinfo>

#include "qasmtools/ast/ast.hpp"

namespace staq {
namespace output {

namespace ast = qasmtools::ast;

/** \brief Equivalent IonQ standard gates for qasm standard gates */
std::unordered_map<std::string, std::string> qasmstd_to_ionq{
    {"sdg", "si"}, {"tdg", "ti"}, {"u1", "rz"}};

/**
 * \class staq::output::IonQOutputter
 * \brief Visitor for converting a QASM AST to IonQ
 */
class IonQOutputter final : public ast::Visitor {
  public:
    IonQOutputter(std::ostream& os) : Visitor(), os_(os) {}
    ~IonQOutputter() = default;

    void run(ast::Program& prog) {
        prefix_ = "";
        first_gate = true;

        prog.accept(*this);
    }

    // Variables
    void visit(ast::VarAccess& ap) {}

    // Expressions
    void visit(ast::BExpr& expr) {}

    void visit(ast::UExpr& expr) {}

    void visit(ast::PiExpr&) {}

    void visit(ast::IntExpr& expr) {}

    void visit(ast::RealExpr& expr) {}

    void visit(ast::VarExpr& expr) {}

    // Statements
    void visit(ast::MeasureStmt& stmt) {}

    void visit(ast::ResetStmt& stmt) {}

    void visit(ast::IfStmt& stmt) {}

    // Gates
    void visit(ast::UGate& gate) {}

    void visit(ast::CNOTGate& gate) {}

    void visit(ast::BarrierGate&) {}

    void visit(ast::DeclaredGate& gate) {
        // JSON output: avoid outputting comma before first gate
        if (first_gate) {
            first_gate = false;
        } else {
            os_ << ",\n";
        }

        os_ << prefix_ << "{\n";
        prefix_ += "    ";

        std::string name = gate.name();
        if (auto it = qasmstd_to_ionq.find(gate.name());
            it != qasmstd_to_ionq.end())
            name = it->second;

        int cur_qarg = 0;
        bool ctrl = false;
        // Handle control gates
        // Control is the first qarg, if applicable
        if (name[0] == 'c') {
            cur_qarg = 1;
            ctrl = true;
            name = name.substr(1);
        }

        // IonQ handles gates with multiple control args,
        //  but the QE standard gates have at most one control arg.
        if (ctrl) {
            os_ << prefix_ << "\"control\": " << gate.qarg(0).offset().value()
                << ",\n";
        }

        if (gate.num_qargs() - cur_qarg == 1) {
            // Single target gate
            os_ << prefix_
                << "\"target\": " << gate.qarg(cur_qarg).offset().value()
                << ",\n";
        } else {
            // Multi target gate
            os_ << prefix_ << "\"targets\": [";
            for (int i = cur_qarg; i < gate.num_qargs(); ++i) {
                os_ << gate.qarg(i).offset().value();
                if (i + 1 != gate.num_qargs())
                    os_ << ",";
            }

            os_ << "],\n";
        }

        if (gate.num_cargs()) {
            // TODO: assert that there is exactly one carg
            // TODO: assert that this is a rotation gate
            // TODO: Handle multiples of pi nicely
            os_ << prefix_ << "\"angle\": " << std::fixed
                << gate.carg(0).constant_eval().value() / M_PI << ",\n";
        }

        os_ << prefix_ << "\"gate\": \"" << name << "\"\n"; // no comma

        prefix_.resize(prefix_.size() - 4);
        os_ << prefix_
            << "}"; // Do not output newline for proper JSON formatting
    }

    // Declarations
    void visit(ast::GateDecl& decl) {}

    void visit(ast::OracleDecl& decl) {}

    void visit(ast::RegisterDecl& decl) {
        if (decl.is_quantum()) {
            os_ << prefix_ << "\"qubits\": " << decl.size() << ",\n";
        } else {
            // should never occur
        }
    }

    void visit(ast::AncillaDecl& decl) {}

    // Program
    void visit(ast::Program& prog) {
        os_ << prefix_ << "{\n";
        prefix_ += "    ";

        os_ << prefix_ << "\"format\": \"ionq.circuit.v0\",\n";
        os_ << prefix_ << "\"gateset\": \"qis\",\n";

        // Print the qubit count line (global register decl)
        prog.foreach_stmt([this](auto& stmt) {
            if (typeid(stmt) == typeid(ast::RegisterDecl))
                stmt.accept(*this);
        });

        os_ << prefix_ << "\"circuit\": [\n";
        prefix_ += "    ";

        // Program body
        prog.foreach_stmt([this](auto& stmt) {
            // Skip the gate declarations from qelib1.inc
            // and the global register decl
            if ((typeid(stmt) != typeid(ast::GateDecl)) &&
                (typeid(stmt) != typeid(ast::RegisterDecl)))
                stmt.accept(*this);
        });

        // Close circuit
        os_ << "\n";
        prefix_.resize(prefix_.size() - 4);
        os_ << prefix_ << "]\n";

        // Close input
        prefix_.resize(prefix_.size() - 4);
        os_ << prefix_ << "}\n";
    }

  private:
    std::ostream& os_;

    std::string prefix_ = "";
    bool first_gate = true;
};

/** \brief Writes an AST in IonQ format to stdout */
void output_ionq(ast::Program& prog) {
    IonQOutputter outputter(std::cout);
    outputter.run(prog);
}

/** \brief Writes an AST in IonQ format to a given output stream */
void write_ionq(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
        std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
        IonQOutputter outputter(ofs);
        outputter.run(prog);
    }

    ofs.close();
}

} /* namespace output */
} /* namespace staq */

#endif /* OUTPUT_IONQ_HPP_ */
