/**
 * \file transformations/synth.hpp
 * \brief []
 */

#ifndef TRANSFORMATIONS_SYNTH_HPP_
#define TRANSFORMATIONS_SYNTH_HPP_

#include <list>
#include <variant>

#include "qasmtools/ast/replacer.hpp"

namespace staq {
namespace transformations {

namespace ast = qasmtools::ast;

void desugar(ast::ASTNode& node);

/* Implementation */
class DesugarImpl final : public ast::Replacer {
  public:
    DesugarImpl() = default;
    ~DesugarImpl() = default;

    void run(ast::ASTNode& node) { node.accept(*this); }


    /* Overrides */

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {

        if (gate.name() == "rz") {
            std::list<ast::ptr<ast::Gate>> ret;
            std::vector<ast::ptr<ast::Expr>> c_args;
            std::vector<ast::VarAccess> q_args(gate.qargs());
            gate.foreach_carg([&c_args, this](auto& arg) {
                c_args.emplace_back(ast::object::clone(arg));

                std::optional<double> val = arg.constant_eval();
                
                std::cerr << "CARG " << arg << ' ' << (val ? val.value() : 0) << std::endl;
            });
            gate.foreach_qarg([](auto& arg){
                std::optional<int> offset = arg.offset();
                std::cerr << "QARG " << arg.var() << ' ' << (offset ? offset.value() : -1) << std::endl;
            });
            ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                ast::DeclaredGate(gate.pos(), "REPLACED_RZ",
                                  std::move(c_args), std::move(q_args))));

            


            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }
};

void desugar(ast::ASTNode& node) {
    DesugarImpl alg;
    alg.run(node);
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_SYNTH_HPP_ */
