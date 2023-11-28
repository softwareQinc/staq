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
 * \file transformations/replace_ugate.hpp
 * \brief Replacing common U gates with QE standard gates
 */

#ifndef TRANSFORMATIONS_REPLACE_UGATE_HPP_
#define TRANSFORMATIONS_REPLACE_UGATE_HPP_

#include <list>
#include <unordered_map>
#include <variant>

#include "qasmtools/ast/replacer.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;

/**
 * \brief Replacing UGates
 *
 * Visits an AST and replaces common U gates with QE standard
 * gates if possible. Assumes qelib1.inc is included.
 */
void replace_ugates(ast::ASTNode& node);

static constexpr double pi = qasmtools::utils::pi;
static constexpr double EPS = 1e-9;

struct UArgs {
    double theta;
    double phi;
    double lambda;
};

// clang-format off
static const std::vector<std::pair<UArgs,std::string>> standard_gates{
    {{pi,   0,    pi},   "x"},
    {{pi,   pi/2, pi/2}, "y"},
    {{0,    0,    pi},   "z"},
    {{pi/2, 0,    pi},   "h"},
    {{0,    0,    pi/2}, "s"},
    {{0,    0,   -pi/2}, "sdg"},
    {{0,    0,    pi/4}, "t"},
    {{0,    0,   -pi/4}, "tdg"}
};
// clang-format on

/* Implementation */
class ReplaceUGateImpl final : public ast::Replacer {
  public:
    ReplaceUGateImpl() = default;
    ~ReplaceUGateImpl() = default;

    void run(ast::ASTNode& node) { node.accept(*this); }

    // Replace ast::CNOT with ast::DeclaredGate cx
    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::CNOTGate& gate) override {
        std::cerr << "CNOT\n";
        std::vector<ast::ptr<ast::Expr>> c_args;
        std::vector<ast::VarAccess> q_args{gate.ctrl(), gate.tgt()};

        std::list<ast::ptr<ast::Gate>> ret;
        ret.emplace_back(std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
            gate.pos(), "cx", std::move(c_args), std::move(q_args))));
        return std::move(ret);
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::UGate& gate) override {
        double theta, phi, lambda;
        try {
            theta = gate.theta().constant_eval().value();
            phi = gate.phi().constant_eval().value();
            lambda = gate.lambda().constant_eval().value();
        } catch (...) {
            std::cerr << "found VarExpr in UGate args"
                      << "\n";
            // this should never happen; if this is reached then the Replacer
            // is recurring too far down the AST
            throw;
        }

        std::string name = "";
        std::vector<ast::ptr<ast::Expr>> c_args;
        std::vector<ast::VarAccess> q_args{gate.arg()};

        // Simple cases: x y z h s sdg t tdg
        for (auto& [g_args, g_name] : standard_gates) {
            if (std::abs(theta - g_args.theta) < EPS &&
                std::abs(phi - g_args.phi) < EPS &&
                std::abs(lambda - g_args.lambda) < EPS) {

                name = g_name;
                break;
            }
        }

        // Remaining cases: rz ry rx
        if (name == "") {
            if (std::abs(theta) < EPS && std::abs(phi) < EPS) {
                name = "rz";
                // TODO: Directly copy the exprs from the U gate
                //  to avoid precision loss
                c_args.emplace_back(
                    std::make_unique<ast::RealExpr>(gate.pos(), lambda));
            } else if (std::abs(phi) < EPS && std::abs(lambda) < EPS) {
                name = "ry";
                c_args.emplace_back(
                    std::make_unique<ast::RealExpr>(gate.pos(), theta));
            } else if (std::abs(phi + pi / 2) < EPS &&
                       std::abs(lambda - pi / 2) < EPS) {
                name = "rx";
                c_args.emplace_back(
                    std::make_unique<ast::RealExpr>(gate.pos(), theta));
            }
        }

        // Throw error if U gate is not a QE standard gate
        if (name == "") {
            throw std::logic_error{""};
            return std::nullopt;
        }

        std::list<ast::ptr<ast::Gate>> ret;
        ret.emplace_back(std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
            gate.pos(), name, std::move(c_args), std::move(q_args))));
        return std::move(ret);
    }

    // Avoid visiting children of GateDecl
    void visit(ast::GateDecl& decl) override {}
};

void replace_ugates(ast::ASTNode& node) {
    ReplaceUGateImpl alg;
    alg.run(node);
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_REPLACE_UGATE_HPP_ */