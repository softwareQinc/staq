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
 * \file optimization/simplify.hpp
 * \brief Gate cancellation optimization
 */

#ifndef OPTIMIZATION_SIMPLIFY_HPP_
#define OPTIMIZATION_SIMPLIFY_HPP_

#include "qasmtools/ast/visitor.hpp"
#include "qasmtools/ast/replacer.hpp"

#include <tuple>

namespace staq {
namespace optimization {

namespace ast = qasmtools::ast;

/**
 * \brief Basic adjacent gate cancellation algorithm
 *
 *  Returns a replacement list giving the nodes to the be erased
 */

// TODO: add option for global phase correction
class Simplifier final : public ast::Visitor {
  public:
    struct config {
        bool fixpoint = true;
    };

    Simplifier() = default;
    Simplifier(const config& params) : Visitor(), config_(params) {}
    ~Simplifier() = default;

    void run(ast::ASTNode& node) {
        do {
            replace_gates(node, std::move(erasures_));
            reset();
            node.accept(*this);
        } while (!erasures_.empty());
    }

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
    void visit(ast::MeasureStmt& stmt) {
        last_[stmt.q_arg()] = {"measure", {stmt.q_arg()}, stmt.uid()};
    }
    void visit(ast::ResetStmt& stmt) {
        last_[stmt.arg()] = {"reset", {stmt.arg()}, stmt.uid()};
    }
    void visit(ast::IfStmt& stmt) {
        mergeable_ = false;
        stmt.then().accept(*this);
        mergeable_ = true;
    }

    /* Gates */
    void visit(ast::UGate& gate) {
        auto theta = gate.theta().constant_eval();
        auto phi = gate.phi().constant_eval();
        auto lambda = gate.lambda().constant_eval();
        if (theta && phi && lambda && (*theta == 0) && (*phi + *lambda == 0)) {
            erasures_[gate.uid()] = std::move(std::list<ast::ptr<ast::Gate>>());
            return;
        }

        last_[gate.arg()] = {"U", {gate.arg()}, gate.uid()};
    }
    void visit(ast::CNOTGate& gate) {
        auto ctrl = gate.ctrl();
        auto tgt = gate.tgt();

        if (mergeable_) {
            auto [name1, args1, uid1] = last_[ctrl];
            auto [name2, args2, uid2] = last_[tgt];

            if (uid1 == uid2 && name1 == "cx" &&
                args1 == std::vector<ast::VarAccess>({ctrl, tgt})) {
                erasures_[uid1] = std::move(std::list<ast::ptr<ast::Gate>>());
                erasures_[gate.uid()] =
                    std::move(std::list<ast::ptr<ast::Gate>>());

                last_.erase(ctrl);
                last_.erase(tgt);

                return;
            }
        }

        last_[ctrl] = {"cx", {ctrl, tgt}, gate.uid()};
        last_[tgt] = {"cx", {ctrl, tgt}, gate.uid()};
    }
    void visit(ast::BarrierGate& gate) {
        gate.foreach_arg([this, &gate](auto& arg) {
            last_[arg] = {"barrier", gate.args(), gate.uid()};
        });
    }
    void visit(ast::DeclaredGate& gate) {
        auto name = gate.name();

        if (name == "u3") {
            auto theta = gate.carg(0).constant_eval();
            auto phi = gate.carg(1).constant_eval();
            auto lambda = gate.carg(2).constant_eval();
            if (theta && phi && lambda && (*theta == 0) &&
                (*phi + *lambda == 0)) {
                erasures_[gate.uid()] =
                    std::move(std::list<ast::ptr<ast::Gate>>());
                return;
            }
        } else if (name == "u1" || name == "rx" || name == "ry" ||
                   name == "rz" || name == "crz" || name == "cu1") {
            auto lambda = gate.carg(0).constant_eval();
            if (lambda && (*lambda == 0)) {
                erasures_[gate.uid()] =
                    std::move(std::list<ast::ptr<ast::Gate>>());
                return;
            }
        } else if (name == "id" || name == "u0") {
            erasures_[gate.uid()] = std::move(std::list<ast::ptr<ast::Gate>>());
            return;
        } else if (name == "cu3") {
            auto theta = gate.carg(0).constant_eval();
            auto phi = gate.carg(1).constant_eval();
            auto lambda = gate.carg(2).constant_eval();
            if (theta && phi && lambda && (*theta == 0) && (*phi == 0) &&
                (*lambda == 0)) {
                erasures_[gate.uid()] =
                    std::move(std::list<ast::ptr<ast::Gate>>());
                return;
            }
        }

        if (mergeable_) {
            if (name == "cx") {
                auto ctrl = gate.qarg(0);
                auto tgt = gate.qarg(1);
                auto [name1, args1, uid1] = last_[ctrl];
                auto [name2, args2, uid2] = last_[tgt];

                if (uid1 == uid2 && name1 == "cx" &&
                    args1 == std::vector<ast::VarAccess>({ctrl, tgt})) {
                    erasures_[uid1] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(ctrl);
                    last_.erase(tgt);

                    return;
                }
            } else if (name == "ccx") {
                auto ctrl1 = gate.qarg(0);
                auto ctrl2 = gate.qarg(1);
                auto tgt = gate.qarg(2);
                auto [name1, args1, uid1] = last_[ctrl1];
                auto [name2, args2, uid2] = last_[ctrl2];
                auto [name3, args3, uid3] = last_[tgt];

                if (uid1 == uid2 && uid1 == uid3 && name1 == "ccx" &&
                    args1 == std::vector<ast::VarAccess>({ctrl1, ctrl2, tgt})) {
                    erasures_[uid1] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(ctrl1);
                    last_.erase(ctrl2);
                    last_.erase(tgt);

                    return;
                }
            } else if (name == "h") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "h" && args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "x") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "x" && args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "y") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "y" && args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "z") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "z" && args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "s") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "sdg" &&
                    args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "sdg") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "s" && args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "t") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "tdg" &&
                    args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            } else if (name == "tdg") {
                auto arg = gate.qarg(0);
                auto [name, args, uid] = last_[arg];

                if (name == "t" && args == std::vector<ast::VarAccess>({arg})) {
                    erasures_[uid] =
                        std::move(std::list<ast::ptr<ast::Gate>>());
                    erasures_[gate.uid()] =
                        std::move(std::list<ast::ptr<ast::Gate>>());

                    last_.erase(arg);

                    return;
                }
            }
        }

        gate.foreach_qarg([this, &gate, &name](auto& arg) {
            last_[arg] = {name, gate.qargs(), gate.uid()};
        });
    }

    /* Declarations */
    void visit(ast::GateDecl& decl) {
        // Initialize a new local state
        std::unordered_map<
            ast::VarAccess,
            std::tuple<std::string, std::vector<ast::VarAccess>, int>>
            local_state;
        std::swap(last_, local_state);

        // Process gate body
        decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });

        // Reset the state
        std::swap(last_, local_state);
    }
    void visit(ast::OracleDecl&) {}
    void visit(ast::RegisterDecl&) {}
    void visit(ast::AncillaDecl&) {}

    /* Program */
    void visit(ast::Program& prog) {
        prog.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
    }

  private:
    config config_;
    bool mergeable_;
    std::unordered_map<int, std::list<ast::ptr<ast::Gate>>> erasures_;
    std::unordered_map<
        ast::VarAccess,
        std::tuple<std::string, std::vector<ast::VarAccess>, int>>
        last_;

    void reset() {
        erasures_.clear();
        last_.clear();
        mergeable_ = true;
    }
};

inline void simplify(ast::ASTNode& node) {
    Simplifier optimizer;
    optimizer.run(node);
}

inline void simplify(ast::ASTNode& node, const Simplifier::config& params) {
    Simplifier optimizer(params);
    optimizer.run(node);
}

} /* namespace optimization */
} /* namespace staq */

#endif /* OPTIMIZATION_SIMPLIFY_HPP_ */
