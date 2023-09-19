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
 * \brief Replace rz gates with approximations
 */

#ifndef TRANSFORMATIONS_QASM_SYNTH_HPP_
#define TRANSFORMATIONS_QASM_SYNTH_HPP_

#include <cstdlib>
#include <list>
#include <variant>

#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/types.hpp"
#include "qasmtools/ast/replacer.hpp"
#include "qasmtools/parser/parser.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;
using real_t = staq::grid_synth::real_t;

/* Implementation */
class ReplaceRZImpl final : public ast::Replacer {
  public:
    ReplaceRZImpl(grid_synth::domega_matrix_table_t& s3_table, real_t& eps,
                  bool check, bool details, bool verbose)
        : s3_table_(s3_table), eps_(eps), check_(check), details_(details),
          verbose_(verbose){};
    ~ReplaceRZImpl() = default;

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
            std::string rz_approx = get_rz_approx(angle);
            if (details_) {
                std::cerr << gate.pos() << ": found approximation " << rz_approx
                          << '\n';
            }

            for (char c : rz_approx) {
                ret.emplace_back(make_gate(std::string(1, tolower(c)), gate));
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

  private:
    real_t& eps_;
    grid_synth::domega_matrix_table_t& s3_table_;
    bool check_;
    bool details_;
    bool verbose_;
    std::unordered_map<std::string, std::string> rz_approx_cache_;

    std::string to_string(const mpf_class& x) const {
        mp_exp_t exp;
        std::string s =
            x.get_str(exp, 32).substr(0, mpf_get_default_prec() / 5);
        return s + std::string(" ") + std::to_string(exp);
    }

    /*!
     * \brief Makes a new gate with no cargs.
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

    /*! \brief Find RZ-approximation for an angle using grid_synth. */
    std::string get_rz_approx(const real_t& angle) {
        using namespace grid_synth;

        if (verbose_)
            std::cerr << "Checking common cases..."
                      << "\n";
        std::string ret = check_common_cases(angle / gmpf::gmp_pi(), eps_);
        if (ret != "") {
            if (details_)
                std::cerr
                    << "Angle is multiple of pi/4, answer is known exactly"
                    << '\n';
            if (check_)
                std::cerr << "Check flag = " << 1 << '\n';
            std::string ret_no_spaces;
            for (char c : ret) {
                if (c != ' ')
                    ret_no_spaces.push_back(c);
            }
            return ret_no_spaces;
        }

        std::string angle_str = to_string(angle);
        if (verbose_) {
            std::cerr << "Checking local cache..." << '\n';
            std::cerr << "Angle has string representation " << angle_str
                      << '\n';
        }
        if (rz_approx_cache_.count(angle_str)) {
            if (verbose_ || details_)
                std::cerr << "Angle is found in local cache" << '\n';
            return rz_approx_cache_[angle_str];
        }

        if (verbose_)
            std::cerr << "Running grid_synth to find new rz approximation..."
                      << '\n';
        RzApproximation rz_approx =
            find_fast_rz_approximation(angle / real_t("-2.0"), eps_);
        if (!rz_approx.solution_found()) {
            std::cerr << "No approximation found for RzApproximation. "
                         "Try changing factorization effort."
                      << '\n';
            exit(EXIT_FAILURE); // TODO: change this to fail more gracefully?
        }
        if (verbose_)
            std::cerr << "Approximation found. Synthesizing..." << '\n';
        ret = synthesize(rz_approx.matrix(), s3_table_);

        if (verbose_)
            std::cerr << "Synthesis complete." << '\n';
        if (check_) {
            std::cerr << "Check flag = "
                      << (rz_approx.matrix() ==
                          domega_matrix_from_str(full_simplify_str(ret)))
                      << '\n';
        }
        if (details_) {
            real_t scale = gmpf::pow(SQRT2, rz_approx.matrix().k());
            std::cerr << "angle = " << std::scientific << angle << '\n';
            std::cerr << rz_approx.matrix();
            std::cerr << "u decimal value = "
                      << "(" << rz_approx.matrix().u().decimal().real() / scale
                      << "," << rz_approx.matrix().u().decimal().imag() / scale
                      << ")" << '\n';
            std::cerr << "t decimal value = "
                      << "(" << rz_approx.matrix().t().decimal().real() / scale
                      << "," << rz_approx.matrix().t().decimal().imag() / scale
                      << ")" << '\n';
            std::cerr << "error = " << rz_approx.error() << '\n';
            str_t simplified = full_simplify_str(ret);
            std::string::difference_type n =
                count(simplified.begin(), simplified.end(), 'T');
            std::cerr << "T count = " << n << '\n';
            std::cerr << "----" << '\n' << std::fixed;
        }

        rz_approx_cache_[angle_str] = ret;
        return ret;
    }
};

void replace_rz(ast::ASTNode& node, grid_synth::domega_matrix_table_t& s3_table,
                real_t& eps, bool check = false, bool details = false,
                bool verbose = false) {
    ReplaceRZImpl alg(s3_table, eps, check, details, verbose);
    alg.run(node);
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_QASM_SYNTH_HPP_ */
