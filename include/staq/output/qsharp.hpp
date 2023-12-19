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
 * \file output/qsharp.hpp
 * \brief Q# outputter
 */

#ifndef OUTPUT_QSHARP_HPP_
#define OUTPUT_QSHARP_HPP_

#include <iomanip>
#include <typeinfo>

#include "qasmtools/ast/ast.hpp"

namespace staq {
namespace output {

namespace ast = qasmtools::ast;

/** \brief Equivalent Q# standard gates for qasm standard gates */
std::unordered_map<std::string, std::string> qasmstd_to_qsharp{
    {"id", "I"},
    {"x", "X"},
    {"y", "Y"},
    {"z", "Z"},
    {"h", "H"},
    {"s", "S"},
    {"sdg", "(Adjoint S)"},
    {"t", "T"},
    {"tdg", "(Adjoint T)"},
    {"cx", "CNOT"},
    {"cz", "CZ"},
    {"ch", "(Controlled H)"},
    {"ccx", "CCNOT"},
    {"rx", "Rx"},
    {"ry", "Ry"},
    {"rz", "Rz"},
    {"u1", "Rz"},
    {"crz", "(Controlled Rz)"},
    {"cu1", "(Controlled Rz)"}};

/**
 * \class staq::output::QSharpOutputter
 * \brief Visitor for converting a QASM AST to Q#
 */
class QSharpOutputter final : public ast::Visitor {
  public:
    struct config {
        bool driver = false;
        std::string ns = "Quantum.staq";
        std::string opname = "Circuit";
    };

    QSharpOutputter(std::ostream& os) : Visitor(), os_(os) {}
    QSharpOutputter(std::ostream& os, const config& params)
        : Visitor(), os_(os), config_(params) {}
    ~QSharpOutputter() = default;

    void run(ast::Program& prog) {
        prefix_ = "";
        ambiguous_ = false;
        locals_.clear();

        prog.accept(*this);
    }

    // Variables
    void visit(ast::VarAccess& ap) { os_ << ap; }

    // Expressions
    void visit(ast::BExpr& expr) {
        auto tmp = ambiguous_;

        if (expr.op() == ast::BinaryOp::Pow) {
            ambiguous_ = false;
            // Override since ^ is strictly integral in Q#
            os_ << "PowD(";
            expr.lexp().accept(*this);
            os_ << ", ";
            expr.rexp().accept(*this);
        } else {
            ambiguous_ = true;
            if (tmp) {
                os_ << "(";
                expr.lexp().accept(*this);
                os_ << expr.op();
                expr.rexp().accept(*this);
                os_ << ")";
            } else {
                expr.lexp().accept(*this);
                os_ << expr.op();
                expr.rexp().accept(*this);
            }
        }
        ambiguous_ = tmp;
    }

    void visit(ast::UExpr& expr) {
        switch (expr.op()) {
            case ast::UnaryOp::Neg: {
                auto tmp = ambiguous_;
                ambiguous_ = true;
                os_ << "-";
                expr.subexp().accept(*this);
                ambiguous_ = tmp;
                break;
            }
            case ast::UnaryOp::Sin:
                os_ << "Sin(";
                expr.subexp().accept(*this);
                os_ << ")";
                break;
            case ast::UnaryOp::Cos:
                os_ << "Cos(";
                expr.subexp().accept(*this);
                os_ << ")";
                break;
            case ast::UnaryOp::Tan:
                os_ << "Tan(";
                expr.subexp().accept(*this);
                os_ << ")";
                break;
            case ast::UnaryOp::Ln:
                os_ << "Log(";
                expr.subexp().accept(*this);
                os_ << ")";
                break;
            case ast::UnaryOp::Sqrt:
                os_ << "Sqrt(";
                expr.subexp().accept(*this);
                os_ << ")";
                break;
            case ast::UnaryOp::Exp:
                os_ << "ExpD(";
                expr.subexp().accept(*this);
                os_ << ")";
                break;
            default:
                break;
        }
    }

    void visit(ast::PiExpr&) { os_ << "PI()"; }

    void visit(ast::IntExpr& expr) { os_ << expr.value() << ".0"; }

    void visit(ast::RealExpr& expr) {
        auto tmp = expr.value();

        os_ << tmp;
        if (tmp - floor(tmp) == 0) {
            os_ << ".0";
        }
    }

    void visit(ast::VarExpr& expr) { os_ << expr.var(); }

    // Statements
    void visit(ast::MeasureStmt& stmt) {
        // Arrays are immutable in Q#
        os_ << prefix_ << "set " << stmt.c_arg().var();
        os_ << " w/= " << *(stmt.c_arg().offset());
        os_ << " <- M(" << stmt.q_arg() << ");\n";
    }

    void visit(ast::ResetStmt& stmt) {
        os_ << prefix_ << "Reset(" << stmt.arg() << ");\n";
    }

    void visit(ast::IfStmt& stmt) {
        os_ << prefix_ << "if (ResultArrayAsInt(" << stmt.var();
        os_ << ") == " << stmt.cond() << ") {\n";

        prefix_ += "    ";
        stmt.then().accept(*this);
        prefix_.resize(prefix_.size() - 4);

        os_ << prefix_ << "}\n";
    }

    // Gates
    void visit(ast::UGate& gate) {
        os_ << prefix_ << "U(";
        gate.theta().accept(*this);
        os_ << ", ";
        gate.phi().accept(*this);
        os_ << ", ";
        gate.lambda().accept(*this);
        os_ << ", ";
        gate.arg().accept(*this);
        os_ << ");\n";
    }

    void visit(ast::CNOTGate& gate) {
        os_ << prefix_ << "CNOT(";
        gate.ctrl().accept(*this);
        os_ << ", ";
        gate.tgt().accept(*this);
        os_ << ");\n";
    }

    void visit(ast::BarrierGate&) {}

    void visit(ast::DeclaredGate& gate) {
        os_ << prefix_;

        if (auto it = qasmstd_to_qsharp.find(gate.name());
            it != qasmstd_to_qsharp.end()) {
            os_ << it->second << "(";
        } else {
            os_ << gate.name() << "(";
        }

        for (int i = 0; i < (gate.num_cargs() + gate.num_qargs()); i++) {
            if (i != 0) {
                os_ << ", ";
            }

            if (i < gate.num_cargs()) {
                gate.carg(i).accept(*this);
            } else {
                gate.qarg(i - gate.num_cargs()).accept(*this);
            }
        }
        os_ << ");\n";
    }

    // Declarations
    void visit(ast::GateDecl& decl) {
        if (decl.is_opaque()) {
            throw std::logic_error("Opaque declarations not supported");
        }

        if (qasmstd_to_qsharp.find(decl.id()) == qasmstd_to_qsharp.end()) {

            // Declaration header
            os_ << prefix_ << "operation " << decl.id() << "(";
            for (int i = 0;
                 i < (decl.c_params().size() + decl.q_params().size()); i++) {
                if (i != 0) {
                    os_ << ", ";
                }

                if (i < decl.c_params().size()) {
                    os_ << decl.c_params()[i] << " : Double";
                } else {
                    os_ << decl.q_params()[i - decl.c_params().size()]
                        << " : Qubit";
                }
            }
            os_ << ") : Unit {\n";

            // Declaration body
            prefix_ += "    ";
            decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });

            // Reset all local ancillas
            for (auto it = locals_.rbegin(); it != locals_.rend(); it++) {
                os_ << prefix_ << "ResetAll(" << *it << ");\n";
                prefix_.resize(prefix_.size() - 4);
                os_ << prefix_ << "}\n";
            }
            locals_.clear();

            prefix_.resize(prefix_.size() - 4);
            os_ << prefix_ << "}\n\n";
        }
    }

    void visit(ast::OracleDecl& decl) {
        throw std::logic_error(
            "Q# has no support for oracle declarations via logic files");
    }

    void visit(ast::RegisterDecl& decl) {
        if (decl.is_quantum()) {
            os_ << prefix_ << "using (" << decl.id() << " = Qubit["
                << decl.size() << "]) {";
            prefix_ += "    ";
            locals_.push_back(decl.id());
        } else {
            os_ << prefix_ << "mutable " << decl.id() << " = new Result["
                << decl.size() << "];";
        }
        os_ << "\n";
    }

    void visit(ast::AncillaDecl& decl) {
        os_ << prefix_ << "using (" << decl.id() << " = Qubit[" << decl.size()
            << "]) {\n";
        prefix_ += "    ";
        locals_.push_back(decl.id());
    }

    // Program
    void visit(ast::Program& prog) {
        os_ << prefix_ << "namespace " << config_.ns << " {\n";
        prefix_ += "    ";

        os_ << prefix_ << "open Microsoft.Quantum.Intrinsic;\n";
        os_ << prefix_ << "open Microsoft.Quantum.Convert;\n";
        os_ << prefix_ << "open Microsoft.Quantum.Canon;\n";
        os_ << prefix_ << "open Microsoft.Quantum.Math;\n\n";

        // QASM U gate
        os_ << prefix_
            << "operation U(theta : Double, phi : Double, lambda : Double, q : "
               "Qubit) : Unit {\n";
        prefix_ += "    ";
        os_ << prefix_ << "Rz(lambda, q);\n";
        os_ << prefix_ << "Ry(theta, q);\n";
        os_ << prefix_ << "Rz(phi, q);\n";
        prefix_.resize(prefix_.size() - 4);
        os_ << prefix_ << "}\n\n";

        // Gate declarations
        prog.foreach_stmt([this](auto& stmt) {
            if (typeid(stmt) == typeid(ast::GateDecl)) {
                stmt.accept(*this);
            }
        });

        // Program body
        os_ << prefix_ << "operation " << config_.opname << "() : Unit {\n";
        prefix_ += "    ";
        prog.foreach_stmt([this](auto& stmt) {
            if (typeid(stmt) != typeid(ast::GateDecl)) {
                stmt.accept(*this);
            }
        });

        // Reset all qubits
        os_ << "\n";
        for (auto it = locals_.rbegin(); it != locals_.rend(); it++) {
            os_ << prefix_ << "ResetAll(" << *it << ");\n";
            prefix_.resize(prefix_.size() - 4);
            os_ << prefix_ << "}\n";
        }
        locals_.clear();

        // Close operation
        prefix_.resize(prefix_.size() - 4);
        os_ << prefix_ << "}\n";

        // Close namespace
        prefix_.resize(prefix_.size() - 4);
        os_ << prefix_ << "}\n";
    }

  private:
    std::ostream& os_;
    config config_;

    std::string prefix_ = "";
    std::list<std::string> locals_{};
    bool ambiguous_ = false;
};

/** \brief Writes an AST in Q# format to stdout */
void output_qsharp(ast::Program& prog) {
    QSharpOutputter outputter(std::cout);
    outputter.run(prog);
}

/** \brief Writes an AST in Q# format to a given output stream */
void write_qsharp(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
        std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
        QSharpOutputter outputter(ofs);
        outputter.run(prog);
    }

    ofs.close();
}

} /* namespace output */
} /* namespace staq */

#endif /* OUTPUT_QSHARP_HPP_ */
