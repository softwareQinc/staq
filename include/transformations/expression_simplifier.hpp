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
 * \file transformations/expression_simplifier.hpp
 * \brief Simplify certain constant expressions
 */

#pragma once

#include "qasmtools/ast/visitor.hpp"
#include "qasmtools/utils/angle.hpp"

#include <cmath>
#include <variant>
#include <optional>

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;

/* Definition: A LinearPiExpr (LPE) is an expression of the form
 *     a*pi/b+c/d   where a,b,c,d are integers
 *
 * Simplification rules:
 *
 *   Binary ops:
 *     LPE + LPE -> LPE
 *     LPE - LPE -> LPE
 *     LPE * LPE -> LPE only if one of the LPEs has no pi term
 *                      (i.e. can't simplify π*π)
 *           ... -> REAL otherwise
 *     LPE / LPE -> LPE only if (i) the denominator has no pi term; or
 *                              (ii) we have (a*pi/b) / (c*pi/d)
 *           ... -> REAL otherwise
 *     LPE ^ LPE -> REAL
 *
 *     LPE [op] REAL -> REAL
 *     REAL [op] LPE -> REAL
 *     REAL [op] REAL -> REAL
 *
 *     When x is neither an LPE nor a REAL expression, if possible we rewrite
 *         0 + x -> x
 *         0 - x -> (-x)
 *         1 * x -> x
 *         x + 0 -> x
 *         x - 0 -> x
 *         x * 1 -> x
 *         x / 1 -> x
 *         x ^ 1 -> x
 *
 *   Unary ops:
 *     (- LPE) -> LPE
 *     Everything else gets simplified to a REAL
 */

class ExprSimplifier final : public ast::Visitor {
    /* Rational numbers */
    class Rational {
        int n_; // numerator
        int d_; // denominator

      public:
        Rational() : n_(0), d_(1) {}
        Rational(int n) : n_(n), d_(1) {}
        Rational(int n, int d) : n_(n), d_(d) {
            if (d == 0)
                throw std::invalid_argument(
                    "Trying to construct rational with denominator 0");
            reduce();
        }

        bool is_zero() const { return n_ == 0; }
        int numerator() const { return n_; }
        int denominator() const { return d_; }
        double value() const { return (double) n_ / (double) d_; }

        Rational operator-() const { return Rational(-n_, d_); }

        Rational& operator+=(const Rational& rhs) {
            int new_n = n_ * rhs.d_ + d_ * rhs.n_;
            d_ *= rhs.d_;
            n_ = new_n;
            reduce();
            return *this;
        }
        Rational& operator-=(const Rational& rhs) {
            *this += -rhs;
            return *this;
        }
        Rational& operator*=(const Rational& rhs) {
            n_ *= rhs.n_;
            d_ *= rhs.d_;
            reduce();
            return *this;
        }
        Rational& operator/=(const Rational& rhs) {
            n_ *= rhs.d_;
            d_ *= rhs.n_;
            reduce();
            return *this;
        }
        friend Rational operator+(const Rational& lhs, const Rational& rhs) {
            auto tmp = lhs;
            tmp += rhs;
            return tmp;
        }
        friend Rational operator-(const Rational& lhs, const Rational& rhs) {
            return lhs + (-rhs);
        }
        friend Rational operator*(const Rational& lhs, const Rational& rhs) {
            auto tmp = lhs;
            tmp *= rhs;
            return tmp;
        }
        friend Rational operator/(const Rational& lhs, const Rational& rhs) {
            auto tmp = lhs;
            tmp /= rhs;
            return tmp;
        }

        ast::ptr<ast::Expr> to_ast() const {
            ast::ptr<ast::Expr> tmp = ast::IntExpr::create({}, std::abs(n_));
            if (n_ < 0) {
                // unary minus
                tmp = ast::UExpr::create({}, ast::UnaryOp::Neg, std::move(tmp));
            }
            // expression is an integer
            if (d_ == 1) {
                return tmp;
            }
            // expression has a demonimator
            return ast::BExpr::create({}, std::move(tmp), ast::BinaryOp::Divide,
                                      ast::IntExpr::create({}, d_));
        }

      private:
        void reduce() {
            if (n_ == 0) {
                d_ = 1;
            } else {
                // bring into lowest form
                auto tmp = std::gcd(n_, d_);
                n_ = n_ / tmp;
                d_ = d_ / tmp;

                // positive denominator
                if (d_ < 0) {
                    n_ = -n_;
                    d_ = -d_;
                }
            }
        }
    };

    /* Class for values of the form
     *     coefficient * X + constant
     * where coefficient and constant are from a field T,
     * and X is an indeterminate
     */
    template <typename T, typename D>
    class LinearExpr {
      protected:
        T coefficient_;
        T constant_;

      public:
        LinearExpr(T coefficient, T constant)
            : coefficient_(coefficient), constant_(constant) {}

        bool is_zero() const {
            return coefficient_.is_zero() && constant_.is_zero();
        }
        const T& coefficient() const { return coefficient_; }
        const T& constant() const { return constant_; }

        D operator-() const { return D(-coefficient_, -constant_); }

        LinearExpr<T, D>& operator+=(const LinearExpr<T, D>& rhs) {
            coefficient_ += rhs.coefficient_;
            constant_ += rhs.constant_;
            return *this;
        }
        LinearExpr<T, D>& operator-=(const LinearExpr<T, D>& rhs) {
            *this += -rhs;
            return *this;
        }
        LinearExpr<T, D>& operator*=(T fac) {
            coefficient_ *= fac;
            constant_ *= fac;
            return *this;
        }
        LinearExpr<T, D>& operator/=(T div) {
            coefficient_ /= div;
            constant_ /= div;
            return *this;
        }

        friend D operator+(const LinearExpr<T, D>& lhs,
                           const LinearExpr<T, D>& rhs) {
            D tmp(lhs.coefficient_, lhs.constant_);
            tmp += rhs;
            return tmp;
        }
        friend D operator-(const LinearExpr<T, D>& lhs,
                           const LinearExpr<T, D>& rhs) {
            return lhs + (-rhs);
        }
        friend std::optional<D> operator*(const LinearExpr<T, D>& lhs,
                                          const LinearExpr<T, D>& rhs) {
            // first factor is constant
            if (lhs.coefficient_.is_zero()) {
                D tmp(rhs.coefficient_, rhs.constant_);
                tmp *= lhs.constant_;
                return tmp;
            } // second factor is constant
            else if (rhs.coefficient_.is_zero()) {
                D tmp(lhs.coefficient_, lhs.constant_);
                tmp *= rhs.constant_;
                return tmp;
            }
            return std::nullopt;
        }
        friend std::optional<D> operator/(const LinearExpr<T, D>& lhs,
                                          const LinearExpr<T, D>& rhs) {
            // divisor is constant
            if (rhs.coefficient_.is_zero()) {
                if (rhs.constant_.is_zero()) {
                    return std::nullopt; // division by zero
                }
                D tmp(lhs.coefficient_, lhs.constant_);
                tmp /= rhs.constant_;
                return tmp;
            } // both dividend and divisor have no constant term
            else if (lhs.constant_.is_zero() && rhs.constant_.is_zero()) {
                return D({}, lhs.coefficient_ / rhs.coefficient_);
            }
            return std::nullopt;
        }
    };

    /* Class for values of the form
     *     a*pi/b + c/d
     * where a,b,c,d are integers
     */
    class LinearPiExpr : public LinearExpr<Rational, LinearPiExpr> {
        using LinearExpr<Rational, LinearPiExpr>::LinearExpr;

      public:
        ast::ptr<ast::Expr> to_ast() {
            if (coefficient_.is_zero()) {
                // no 'pi' term
                return constant_.to_ast();
            }
            ast::ptr<ast::Expr> tmp = ast::PiExpr::create({});
            // numerator
            int a = coefficient_.numerator();
            if (a == 1) {
            } // multiplication by 1 is omitted
            else if (a == -1) {
                // -pi
                tmp = ast::UExpr::create({}, ast::UnaryOp::Neg, std::move(tmp));
            } else {
                ast::ptr<ast::Expr> a_ast =
                    ast::IntExpr::create({}, std::abs(a));
                if (a < 0) {
                    // unary minus
                    a_ast = ast::UExpr::create({}, ast::UnaryOp::Neg,
                                               std::move(a_ast));
                }
                // a*pi
                tmp = ast::BExpr::create({}, std::move(a_ast),
                                         ast::BinaryOp::Times, std::move(tmp));
            }
            // denominator
            int b = coefficient_.denominator();
            if (b != 1) {
                // a*pi/b
                tmp = ast::BExpr::create({}, std::move(tmp),
                                         ast::BinaryOp::Divide,
                                         ast::IntExpr::create({}, std::abs(b)));
            }
            // constant term
            if (constant_.is_zero()) {
                // + 0
                return tmp;
            } else if (constant_.numerator() < 0) {
                // - c/d
                return ast::BExpr::create({}, std::move(tmp),
                                          ast::BinaryOp::Minus,
                                          (-constant_).to_ast());
            } else {
                // + c/d
                return ast::BExpr::create({}, std::move(tmp),
                                          ast::BinaryOp::Plus,
                                          constant_.to_ast());
            }
        }

        double value() {
            return coefficient_.value() * qasmtools::utils::pi +
                   constant_.value();
        }
    };

    using Expression = std::variant<std::monostate, double, LinearPiExpr>;

    Expression temp_value;
    std::optional<ast::ptr<ast::Expr>> replacement_expr;
    bool evaluate_all;

  public:
    ExprSimplifier(bool evaluate_all = false) : evaluate_all(evaluate_all) {}

    // Variables
    void visit(ast::VarAccess&) {}

    // Expressions
    // - Set temp_value to the value of the expression
    void visit(ast::BExpr& expr) {
        expr.lexp().accept(*this);
        if (replacement_expr) {
            expr.set_lexp(std::move(*replacement_expr));
            replacement_expr = std::nullopt;
        }
        auto lval = temp_value;
        temp_value = std::monostate();

        expr.rexp().accept(*this);
        if (replacement_expr) {
            expr.set_rexp(std::move(*replacement_expr));
            replacement_expr = std::nullopt;
        }
        auto rval = temp_value;
        temp_value = std::monostate();

        std::visit(
            qasmtools::utils::overloaded{
                [this, &expr](LinearPiExpr& lpe1, LinearPiExpr& lpe2) {
                    switch (expr.op()) {
                        case ast::BinaryOp::Plus:
                            temp_value = lpe1 + lpe2;
                            break;
                        case ast::BinaryOp::Minus:
                            temp_value = lpe1 - lpe2;
                            break;
                        case ast::BinaryOp::Times: {
                            auto prod = lpe1 * lpe2;
                            if (prod)
                                temp_value = *prod;
                            else
                                temp_value = lpe1.value() * lpe2.value();
                            break;
                        }
                        case ast::BinaryOp::Divide: {
                            auto quot = lpe1 / lpe2;
                            if (quot)
                                temp_value = *quot;
                            else
                                temp_value = lpe1.value() / lpe2.value();
                            break;
                        }
                        case ast::BinaryOp::Pow:
                            temp_value = pow(lpe1.value(), lpe2.value());
                            break;
                    }
                },
                [this, &expr](LinearPiExpr& lpe1, double real2) {
                    temp_value = evaluate_double_bexpr(lpe1.value(), expr.op(),
                                                       real2);
                },
                [this, &expr](LinearPiExpr& lpe1, auto) {
                    switch (expr.op()) {
                        case ast::BinaryOp::Plus:
                            if (lpe1.is_zero()) // 0 + x
                                replacement_expr =
                                    ast::object::clone(expr.rexp());
                            else
                                expr.set_lexp(lpe1.to_ast());
                            break;
                        case ast::BinaryOp::Minus:
                            if (lpe1.is_zero()) // 0 - x
                                replacement_expr = ast::UExpr::create(
                                    {}, ast::UnaryOp::Neg,
                                    ast::object::clone(expr.rexp()));
                            else
                                expr.set_lexp(lpe1.to_ast());
                            break;
                        case ast::BinaryOp::Times:
                            if (lpe1.value() == 1) // 1 * x
                                replacement_expr =
                                    ast::object::clone(expr.rexp());
                            else
                                expr.set_lexp(lpe1.to_ast());
                            break;
                        case ast::BinaryOp::Divide:
                            expr.set_lexp(lpe1.to_ast());
                            break;
                        case ast::BinaryOp::Pow:
                            expr.set_lexp(lpe1.to_ast());
                            break;
                    }
                },
                [this, &expr](double real1, LinearPiExpr& lpe2) {
                    temp_value = evaluate_double_bexpr(real1, expr.op(),
                                                       lpe2.value());
                },
                [this, &expr](double real1, double real2) {
                    temp_value = evaluate_double_bexpr(real1, expr.op(), real2);
                },
                [this, &expr](double real1, auto) {
                    switch (expr.op()) {
                        case ast::BinaryOp::Plus:
                            if (real1 == 0) // 0 + x
                                replacement_expr =
                                    ast::object::clone(expr.rexp());
                            else
                                expr.set_lexp(ast::RealExpr::create({}, real1));
                            break;
                        case ast::BinaryOp::Minus:
                            if (real1 == 0) // 0 - x
                                replacement_expr = ast::UExpr::create(
                                    {}, ast::UnaryOp::Neg,
                                    ast::object::clone(expr.rexp()));
                            else
                                expr.set_lexp(ast::RealExpr::create({}, real1));
                            break;
                        case ast::BinaryOp::Times:
                            if (real1 == 1) // 1 * x
                                replacement_expr =
                                    ast::object::clone(expr.rexp());
                            else
                                expr.set_lexp(ast::RealExpr::create({}, real1));
                            break;
                        case ast::BinaryOp::Divide:
                            expr.set_lexp(ast::RealExpr::create({}, real1));
                            break;
                        case ast::BinaryOp::Pow:
                            expr.set_lexp(ast::RealExpr::create({}, real1));
                            break;
                    }
                },
                [this, &expr](auto, LinearPiExpr& lpe2) {
                    switch (expr.op()) {
                        case ast::BinaryOp::Plus:
                        case ast::BinaryOp::Minus:
                            if (lpe2.is_zero()) // x + 0  or  x - 0
                                replacement_expr =
                                    ast::object::clone(expr.lexp());
                            else
                                expr.set_rexp(lpe2.to_ast());
                            break;
                        case ast::BinaryOp::Times:
                        case ast::BinaryOp::Divide:
                        case ast::BinaryOp::Pow:
                            if (lpe2.value() == 1) // x * 1 or x / 1 or x ^ 1
                                replacement_expr =
                                    ast::object::clone(expr.lexp());
                            else
                                expr.set_rexp(lpe2.to_ast());
                            break;
                    }
                },
                [this, &expr](auto, double real2) {
                    switch (expr.op()) {
                        case ast::BinaryOp::Plus:
                        case ast::BinaryOp::Minus:
                            if (real2 == 0) // x + 0  or  x - 0
                                replacement_expr =
                                    ast::object::clone(expr.lexp());
                            else
                                expr.set_rexp(ast::RealExpr::create({}, real2));
                            break;
                        case ast::BinaryOp::Times:
                        case ast::BinaryOp::Divide:
                        case ast::BinaryOp::Pow:
                            if (real2 == 1) // x * 1 or x / 1 or x ^ 1
                                replacement_expr =
                                    ast::object::clone(expr.lexp());
                            else
                                expr.set_rexp(ast::RealExpr::create({}, real2));
                            break;
                    }
                },
                [](auto, auto) {}
            },
            lval,
            rval);
    }

    void visit(ast::UExpr& expr) {
        expr.subexp().accept(*this);
        if (replacement_expr) {
            expr.set_subexp(std::move(*replacement_expr));
            replacement_expr = std::nullopt;
        }
        auto val = temp_value;
        temp_value = std::monostate();

        std::visit(
            qasmtools::utils::overloaded{
                [this, &expr](LinearPiExpr& lpe) {
                    switch (expr.op()) {
                        case ast::UnaryOp::Neg:
                            temp_value = -lpe;
                            break;
                        default:
                            // evaluate as real expression
                            temp_value = evaluate_double_uexpr(expr.op(),
                                                               lpe.value());
                    }
                },
                [this, &expr](double real) {
                    temp_value = evaluate_double_uexpr(expr.op(), real);
                },
                [](auto) {}
            },
            val);
    }

    void visit(ast::PiExpr&) {
        if (evaluate_all) {
            temp_value = qasmtools::utils::pi;
        } else {
            temp_value = LinearPiExpr(1, 0);
        }
    }

    void visit(ast::IntExpr& expr) {
        if (evaluate_all) {
            temp_value = (double) expr.value();
        } else {
            temp_value = LinearPiExpr(0, expr.value());
        }
    }

    void visit(ast::RealExpr& expr) { temp_value = expr.value(); }

    void visit(ast::VarExpr&) { temp_value = std::monostate(); }

    // Statements
    void visit(ast::MeasureStmt&) {}
    void visit(ast::ResetStmt&) {}

    void visit(ast::IfStmt& stmt) { stmt.then().accept(*this); }

    // Gates
    void visit(ast::UGate& gate) {
        gate.theta().accept(*this);
        if (replacement_expr) {
            gate.set_theta(std::move(*replacement_expr));
            replacement_expr = std::nullopt;
        } else if (std::holds_alternative<LinearPiExpr>(temp_value)) {
            gate.set_theta(std::get<LinearPiExpr>(temp_value).to_ast());
        } else if (std::holds_alternative<double>(temp_value)) {
            gate.set_theta(
                ast::RealExpr::create({}, std::get<double>(temp_value)));
        }
        temp_value = std::monostate();

        gate.phi().accept(*this);
        if (replacement_expr) {
            gate.set_phi(std::move(*replacement_expr));
            replacement_expr = std::nullopt;
        } else if (std::holds_alternative<LinearPiExpr>(temp_value)) {
            gate.set_phi(std::get<LinearPiExpr>(temp_value).to_ast());
        } else if (std::holds_alternative<double>(temp_value)) {
            gate.set_phi(
                ast::RealExpr::create({}, std::get<double>(temp_value)));
        }
        temp_value = std::monostate();

        gate.lambda().accept(*this);
        if (replacement_expr) {
            gate.set_lambda(std::move(*replacement_expr));
            replacement_expr = std::nullopt;
        } else if (std::holds_alternative<LinearPiExpr>(temp_value)) {
            gate.set_lambda(std::get<LinearPiExpr>(temp_value).to_ast());
        } else if (std::holds_alternative<double>(temp_value)) {
            gate.set_lambda(
                ast::RealExpr::create({}, std::get<double>(temp_value)));
        }
        temp_value = std::monostate();
    }

    void visit(ast::CNOTGate&) {}
    void visit(ast::BarrierGate&) {}

    void visit(ast::DeclaredGate& gate) {
        for (int i = 0; i < gate.num_cargs(); i++) {
            gate.carg(i).accept(*this);
            if (replacement_expr) {
                gate.set_carg(i, std::move(*replacement_expr));
                replacement_expr = std::nullopt;
            } else if (std::holds_alternative<LinearPiExpr>(temp_value)) {
                gate.set_carg(i, std::get<LinearPiExpr>(temp_value).to_ast());
            } else if (std::holds_alternative<double>(temp_value)) {
                gate.set_carg(
                    i, ast::RealExpr::create({}, std::get<double>(temp_value)));
            }
            temp_value = std::monostate();
        }
    }

    // Declarations
    void visit(ast::GateDecl& decl) {
        decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
    }
    void visit(ast::OracleDecl&) {}
    void visit(ast::RegisterDecl&) {}
    void visit(ast::AncillaDecl&) {}

    // Program
    void visit(ast::Program& prog) {
        prog.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
    }

  private:
    /* evaluate binary operation between two doubles */
    double evaluate_double_bexpr(double lhs, ast::BinaryOp op, double rhs) {
        switch (op) {
            case ast::BinaryOp::Plus:
                return lhs + rhs;
            case ast::BinaryOp::Minus:
                return lhs - rhs;
            case ast::BinaryOp::Times:
                return lhs * rhs;
            case ast::BinaryOp::Divide:
                return lhs / rhs;
            case ast::BinaryOp::Pow:
                return pow(lhs, rhs);
        }
        throw std::logic_error("Unrecognized ast::BinaryOp");
    }

    /* evaluate unary operation on a double */
    double evaluate_double_uexpr(ast::UnaryOp op, double val) {
        switch (op) {
            case ast::UnaryOp::Neg:
                return -val;
            case ast::UnaryOp::Sin:
                return std::sin(val);
            case ast::UnaryOp::Cos:
                return std::cos(val);
            case ast::UnaryOp::Tan:
                return std::tan(val);
            case ast::UnaryOp::Ln:
                return std::log(val);
            case ast::UnaryOp::Sqrt:
                return std::sqrt(val);
            case ast::UnaryOp::Exp:
                return std::exp(val);
        }
        throw std::logic_error("Unrecognized ast::UnaryOp");
    }
};

inline void expr_simplify(ast::ASTNode& node, bool evaluate_all = false) {
    ExprSimplifier es(evaluate_all);
    node.accept(es);
}

} // namespace transformations
} // namespace staq
