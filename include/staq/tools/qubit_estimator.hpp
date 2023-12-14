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
 * \file tools/qubit_estimator.hpp
 * \brief Resource estimation
 */

#ifndef TOOLS_QUBIT_ESTIMATOR_HPP_
#define TOOLS_QUBIT_ESTIMATOR_HPP_

#include "qasmtools/ast/ast.hpp"

namespace staq {
namespace tools {

namespace ast = qasmtools::ast;

class QubitEstimator final : public ast::Visitor {
    int qubits_;

  public:
    QubitEstimator() : qubits_(0) {}
    ~QubitEstimator() = default;

    int qubits() { return qubits_; }

    /* Variables */
    void visit(ast::VarAccess&) {}

    /* Expressions */
    void visit(ast::BExpr&) {}
    void visit(ast::UExpr&) {}
    void visit(ast::PiExpr&) {}
    void visit(ast::IntExpr&) {}
    void visit(ast::RealExpr&) {}
    void visit(ast::VarExpr&) {}

    /* Statements */
    void visit(ast::MeasureStmt& stmt) {}
    void visit(ast::ResetStmt& stmt) {}
    void visit(ast::IfStmt& stmt) { stmt.then().accept(*this); }

    /* Gates */
    void visit(ast::UGate& gate) {}
    void visit(ast::CNOTGate& gate) {}
    void visit(ast::BarrierGate& gate) {}
    void visit(ast::DeclaredGate& gate) {}

    /* Declarations */
    void visit(ast::GateDecl& decl) {}
    void visit(ast::OracleDecl&) {}
    void visit(ast::RegisterDecl& decl) {
        if (decl.is_quantum())
            qubits_ += decl.size();
    }
    void visit(ast::AncillaDecl& decl) {
        // count all ancillas as freshly allocated
        qubits_ += decl.size();
    }

    /* Program */
    void visit(ast::Program& prog) {
        prog.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
    }
};

int estimate_qubits(ast::ASTNode& node) {
    QubitEstimator estimator;
    node.accept(estimator);
    return estimator.qubits();
}

} /* namespace tools */
} /* namespace staq */

#endif /* TOOLS_QUBIT_ESTIMATOR_HPP_ */
