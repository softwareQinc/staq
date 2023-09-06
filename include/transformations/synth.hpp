/**
 * \file transformations/synth.hpp
 * \brief Replace rz gates with approximations
 */

#ifndef TRANSFORMATIONS_SYNTH_HPP_
#define TRANSFORMATIONS_SYNTH_HPP_

#include <list>
#include <variant>

#include "qasmtools/ast/replacer.hpp"
#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/rz_approximation.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;

/* Implementation */
class ReplaceRZImpl final : public ast::Replacer {
  public:
    ReplaceRZImpl() = default;
    ~ReplaceRZImpl() = default;

    void run(ast::ASTNode& node) { node.accept(*this); }


    /* Overrides */

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {

        if (gate.name() == "rz") {

            // We assume that the instruction has the form
            //   rz[carg0] qarg0[offset];
            // TODO: assert this

            std::list<ast::ptr<ast::Gate>> ret;

            // Evaluate the Expr as a C++ double for now
            ast::Expr& theta_arg = gate.carg(0);
            std::optional<double> val = theta_arg.constant_eval();
            double value = val ? val.value() : 0;   // this should never be false, TODO: check!
            std::string rz_approx = get_rz_approx(value);
            
            for (char c : rz_approx) {
                if (c == ' ') continue;
                std::vector<ast::ptr<ast::Expr>> c_args;
                std::vector<ast::VarAccess> q_args(gate.qargs());
                
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(gate.pos(), std::string(1, tolower(c)),
                                    std::move(c_args), std::move(q_args))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

    void init() {
        using namespace grid_synth;

        int prec = 17;

        // Initialize constants
        DEFAULT_GMP_PREC = 4 * prec + 19;
        mpf_set_default_prec(log(10)/log(2) * DEFAULT_GMP_PREC);
        //mpf_set_default_prec(log2(10) * DEFAULT_GMP_PREC);
        TOL = pow(real_t(10), -DEFAULT_GMP_PREC + 2);
        PI = gmp_pi(TOL);
        SQRT2 = sqrt(real_t(2));
        INV_SQRT2 = real_t(real_t(1) / SQRT2);
        HALF_INV_SQRT2 = real_t(real_t(1) / (real_t(2) * SQRT2));
        OMEGA = cplx_t(INV_SQRT2, INV_SQRT2);
        OMEGA_CONJ = cplx_t(INV_SQRT2, -INV_SQRT2);
        LOG_LAMBDA = log10(LAMBDA.decimal());
        SQRT_LAMBDA = sqrt(LAMBDA.decimal());
        SQRT_LAMBDA_INV = sqrt(LAMBDA_INV.decimal());
        Im = cplx_t(real_t(0), real_t(1));
        //eps = pow(real_t(10), -prec);

        // Load s3_table
        if (std::ifstream(DEFAULT_TABLE_FILE)) {
            s3_table = read_s3_table(DEFAULT_TABLE_FILE);
        } else {
            s3_table = generate_s3_table();
            write_s3_table(DEFAULT_TABLE_FILE, s3_table);
        }
    }

  private:
    const double eps = 1e-15;
    std::unordered_map<double, std::string> rz_approx_cache;
    grid_synth::domega_matrix_table_t s3_table;

    /*! \brief Find RZ-approximation for angle theta using grid_synth. */
    std::string get_rz_approx(double theta) {
        using namespace grid_synth;
 
        // first check common cases
        std::string ret = check_common_cases(real_t(theta/M_PI), real_t(eps));
        if (ret != "") return ret;

        // then look it up in cache
        if (rz_approx_cache.count(theta)) {
            return rz_approx_cache[theta];
        }
        
        // then actually find an approximation
        RzApproximation rz_approx = find_fast_rz_approximation(real_t(theta)/real_t("-2.0"), real_t(eps));
        ret = synthesize(rz_approx.matrix(), s3_table);
        rz_approx_cache[theta] = ret;
        return ret;
    }

};

void replace_rz(ast::ASTNode& node) {
    ReplaceRZImpl alg;
    alg.init();
    alg.run(node);
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_SYNTH_HPP_ */
