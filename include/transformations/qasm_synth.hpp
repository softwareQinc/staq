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

#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/types.hpp"
#include "qasmtools/ast/replacer.hpp"
#include "qasmtools/parser/parser.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;
using real_t = staq::grid_synth::real_t;
using str_t = staq::grid_synth::str_t;

struct QASMSynthOptions {
    long int prec;
    int factor_effort;
    str_t tablefile;
    bool fact_eff;
    bool read;
    bool write;
    bool check;
    bool details;
    bool verbose;
};

/* Implementation */
class QASMSynthImpl final : public ast::Replacer {
  public:
    QASMSynthImpl(grid_synth::domega_matrix_table_t& s3_table, real_t& eps,
                  const QASMSynthOptions& opt)
        : s3_table_(s3_table), eps_(eps), check_(opt.check),
          details_(opt.details), verbose_(opt.verbose), w_count_(0){};
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
            std::string rz_approx = get_rz_approx(angle);
            if (details_) {
                std::cerr << gate.pos() << ": found approximation " << rz_approx
                          << '\n';
            }

            for (char c : rz_approx) {
                if (c == 'w' || // If the approximation creates w or W gates,
                    c == 'W') { // collect them and output the global phase
                                // later.
                    if (gate.qargs()[0].offset() == std::nullopt) {
                        std::cerr << "Please inline the qasm code first."
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
        int a = (w_count_ % 16 + 16) % 16; // Normalize a to the range [0, 16)
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

  private:
    real_t& eps_;
    grid_synth::domega_matrix_table_t& s3_table_;
    bool check_;
    bool details_;
    bool verbose_;
    std::unordered_map<std::string, std::string> rz_approx_cache_;
    int w_count_;

    /**
     * Converts a GMP float to a string representation suitable for hashing.
     */
    std::string to_string(const mpf_class& x) const {
        mp_exp_t exp;
        // Use base 32 to get a shorter string.
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

/**
 * Replaces all rx/ry/rz gates in a program with grid_synth approximations.
 */
void qasm_synth(ast::ASTNode& node, const QASMSynthOptions& opt) {
    using namespace grid_synth;
    domega_matrix_table_t s3_table;
    real_t eps;

    if (opt.read) {
        if (opt.verbose) {
            std::cerr << "Reading s3_table from " << opt.tablefile << '\n';
        }
        s3_table = read_s3_table(opt.tablefile);
    } else if (opt.write) {
        if (opt.verbose) {
            std::cerr << "Generating new table file and writing to "
                      << opt.tablefile << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(opt.tablefile, s3_table);
    } else if (std::ifstream(DEFAULT_TABLE_FILE)) {
        if (opt.verbose) {
            std::cerr << "Table file found at default location "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if (opt.verbose) {
            std::cerr << "Failed to find " << DEFAULT_TABLE_FILE
                      << ". Generating new table file and writing to "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE, s3_table);
    }

    MP_CONSTS = initialize_constants(opt.prec);
    eps = gmpf::pow(real_t(10), -opt.prec);
    MAX_ATTEMPTS_POLLARD_RHO = opt.factor_effort;

    if (opt.verbose) {
        std::cerr << "Runtime Parameters" << '\n';
        std::cerr << "------------------" << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "TOL (Tolerance for float equality) " << std::setw(1)
                  << ": " << std::setw(3 * COLW) << std::left << std::scientific
                  << TOL << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "KMIN (Minimum scaling exponent) " << std::setw(1) << ": "
                  << std::setw(3 * COLW) << std::left << std::fixed << KMIN
                  << '\n';
        std::cerr << std::setw(2 * COLW) << std::left
                  << "KMAX (Maximum scaling exponent) " << std::setw(1) << ": "
                  << std::setw(3 * COLW) << std::left << std::fixed << KMAX
                  << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "MAX_ATTEMPTS_POLLARD_RHO (How hard we try to factor) "
                  << std::setw(1) << ": " << std::setw(3 * COLW) << std::left
                  << MAX_ATTEMPTS_POLLARD_RHO << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "MAX_ITERATIONS_FERMAT_TEST (How hard we try to check "
                     "primality) "
                  << std::setw(1) << ": " << std::setw(3 * COLW) << std::left
                  << MAX_ITERATIONS_FERMAT_TEST << '\n';
    }
    std::cerr << std::scientific;

    QASMSynthImpl alg(s3_table, eps, opt);
    alg.run(node);
    alg.print_global_phase();
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_QASM_SYNTH_HPP_ */
