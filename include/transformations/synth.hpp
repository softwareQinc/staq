/**
 * \file transformations/synth.hpp
 * \brief Replace rz gates with approximations
 */

#ifndef TRANSFORMATIONS_SYNTH_HPP_
#define TRANSFORMATIONS_SYNTH_HPP_

#include <list>
#include <variant>

#include "grid_synth/exact_synthesis.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "qasmtools/ast/replacer.hpp"

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

        if (gate.name() == "rx" || gate.name() == "ry" || gate.name() == "rz") {

            // We assume that the instruction has the form
            //   rz[carg0] qarg0;
            // TODO: assert this
            // where carg0 does not contain a VarExpr child

            std::list<ast::ptr<ast::Gate>> ret;

            // Evaluate the Expr as a C++ double for now
            ast::Expr& theta_arg = gate.carg(0);
            std::optional<real_t> val2 = theta_arg.constant_eval_gmp();
            // this should never be false, TODO: check!
            real_t value2 = val2 ? val2.value() : real_t(0);
            std::string rz_approx = get_rz_approx(value2);
            std::cerr << value2 << ' ' << rz_approx << std::endl;

            for (char c : rz_approx) {
                if (c == ' ')
                    continue;
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

    void init() {
        using namespace grid_synth;

        int prec = 17;

        // Initialize constants
        DEFAULT_GMP_PREC = 4 * prec + 19;
        mpf_set_default_prec(log(10) / log(2) * DEFAULT_GMP_PREC);
        // mpf_set_default_prec(log2(10) * DEFAULT_GMP_PREC);
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
        // eps = pow(real_t(10), -prec);

        // Load s3_table
        if (std::ifstream(DEFAULT_TABLE_FILE)) {
            s3_table = read_s3_table(DEFAULT_TABLE_FILE);
        } else {
            s3_table = generate_s3_table();
            write_s3_table(DEFAULT_TABLE_FILE, s3_table);
        }
    }

  private:
    const double eps = 1e-10;
    grid_synth::domega_matrix_table_t s3_table;

    struct mpf_class_hash {
        std::size_t operator()(const mpf_class& x) const {
            mp_exp_t exp;
            std::string s = x.get_str(exp, 36);
            // TODO: consider truncating s to a fixed length depending on our prec
            std::string hash_str = s + std::string(" ") + std::to_string(exp);
            return std::hash<std::string>{}(hash_str);
        }
    };
    
    std::unordered_map<mpf_class, std::string, mpf_class_hash> rz_approx_cache;


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

    /*! \brief Find RZ-approximation for angle theta using grid_synth. */
    std::string get_rz_approx(const mpf_class& theta) {
        using namespace grid_synth;

        // first check common cases
        std::string ret =
            check_common_cases(theta / gmp_pi(real_t("0.0000000000000001")),
                               real_t("0.0000000000000001"));
        if (ret != "")
            return ret;

        // then look it up in cache
        if (rz_approx_cache.count(theta)) {
            return rz_approx_cache[theta];
        }

        // then actually find an approximation
        RzApproximation rz_approx = find_fast_rz_approximation(
            theta / real_t("-2.0"), real_t("0.0000000000000001"));
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
