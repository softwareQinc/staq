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
 * \file transformations/qasm_synth.hpp
 * \brief Replace rx/ry/rz gates with grid_synth approximations.
 */

#ifndef TRANSFORMATIONS_QASM_SYNTH_HPP_
#define TRANSFORMATIONS_QASM_SYNTH_HPP_

#include <cstdlib>
#include <list>

#include "staq/grid_synth/exact_synthesis.hpp"
#include "staq/grid_synth/grid_synth.hpp"
#include "staq/grid_synth/rz_approximation.hpp"
#include "staq/grid_synth/types.hpp"

#include "qasmtools/ast/replacer.hpp"
#include "qasmtools/parser/parser.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;
using namespace grid_synth;

/* Implementation */
class QASMSynthImpl final : public ast::Replacer {
  public:
    QASMSynthImpl(const GridSynthOptions& opt)
        : synthesizer_(make_synthesizer(opt)), w_count_(0), check_(opt.check),
          details_(opt.details), verbose_(opt.verbose){};
    ~QASMSynthImpl() = default;

    void run(ast::ASTNode& node) { node.accept(*this); }

    /* Overrides */

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {

        if (gate.name() == "rx" || gate.name() == "ry" || gate.name() == "rz") {

            // By the standard qasm header, these instructions have the form
            //   rz[carg0] qarg0;
            // where carg0 does not contain a VarExpr child.
            // This is checked during the semantic analysis phase of parsing.

            std::list<ast::ptr<ast::Gate>> ret;

            if (verbose_) {
                std::cerr << gate.pos() << ": found gate " << gate.name()
                          << '\n';
            }
            ast::Expr& theta_arg = gate.carg(0);
            std::optional<real_t> expr_val = theta_arg.constant_eval_gmp();
            if (!expr_val) {
                std::cerr << gate.pos() << ": VarExpr found in classical arg0, "
                          << "please inline the code.\n";
                throw qasmtools::parser::ParseError();
            }
            real_t angle = expr_val.value();
            if (details_) {
                std::cerr << gate.pos() << ": gate " << gate.name()
                          << " has angle = " << std::fixed << (angle) << '\n';
            }
            if (verbose_) {
                std::cerr << gate.pos()
                          << ": finding approximation for angle = " << (angle)
                          << '\n';
            }
            std::string op_str = synthesizer_.get_op_str(angle);
            if (details_) {
                std::cerr << gate.pos() << ": found approximation " << op_str
                          << '\n';
            }

            for (char c : op_str) {
                if (c == 'w' || // If the approximation creates w or W gates,
                    c == 'W') { // collect them and output the global phase
                                // later.
                    if (gate.qargs()[0].offset() == std::nullopt) {
                        std::cerr << "Please inline the qasm code first and "
                                     "clear declarations."
                                  << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if (c == 'w') {
                        w_count_ -= 1;
                    } else {
                        w_count_ += 2;
                    }
                } else {
                    ret.emplace_back(
                        make_gate(std::string(1, tolower(c)), gate));
                }
            }

            if (gate.name() == "rx") { // X = HZH
                ret.emplace_front(make_gate("h", gate));
                ret.emplace_back(make_gate("h", gate));
            } else if (gate.name() == "ry") { // Y = SHZHSdg
                ret.emplace_front(make_gate("h", gate));
                ret.emplace_back(make_gate("h", gate));
                ret.emplace_front(make_gate("s", gate));
                ret.emplace_back(make_gate("sdg", gate));
            }

            return std::move(ret);

        } else {
            return std::nullopt;
        }
    }

    /**
     * Prints the global phase of the file.
     * This accounts for all collected w and W gates.
     */
    void print_global_phase() {
        int a = get_w_count();
        if (a == 0)
            return;
        std::cout << "// global-phase: exp i*pi " << a << " " << 8 << std::endl;

#if 0 /* Unused code for outputting the global phase a/8 in least terms. */
        int b = 8; // We simplify a/b.
        int g = std::gcd(a, b);
        a /= g;
        b /= g;
        if (b == 1) { // a == b == 1 in this case
            std::cout << "// global phase: exp(i*pi)" << std::endl;
        } else if (a == 1) {
            std::cout << "// global phase: exp(i*pi / " << b << ")"
                      << std::endl;
        } else {
            std::cout << "// global phase: exp(i*pi * " << a << "/" << b << ")"
                      << std::endl;
        }
#endif
    }

    // Normalize to the range [0, 16)
    int get_w_count() const { return (w_count_ % 16 + 16) % 16; }

  private:
    GridSynthesizer synthesizer_;
    int w_count_;
    bool check_;
    bool details_;
    bool verbose_;

    /*!
     * \brief Copies a gate; gives it a new name and no cargs.
     *
     * \param name The name of the new gate.
     * \param gate The gate to make a copy of.
     */
    inline ast::ptr<ast::Gate> make_gate(std::string name,
                                         ast::DeclaredGate& gate) {
        std::vector<ast::ptr<ast::Expr>> c_args;
        std::vector<ast::VarAccess> q_args(gate.qargs());
        return std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
            gate.pos(), name, std::move(c_args), std::move(q_args)));
    }
};

/**
 * Replaces all rx/ry/rz gates in a program with grid_synth approximations.
 */
int qasm_synth(ast::ASTNode& node, const GridSynthOptions& opt) {
    QASMSynthImpl alg(opt);
    alg.run(node);
    alg.print_global_phase();
    return alg.get_w_count();
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_QASM_SYNTH_HPP_ */
