/*
 * 
 */

/**
 * \file transformations/synth.hpp
 * \brief []
 */

#ifndef TRANSFORMATIONS_SYNTH_HPP_
#define TRANSFORMATIONS_SYNTH_HPP_

#include <list>
#include <unordered_map>
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

    // Necessary since replacer is strictly post-order
    void visit(ast::Program& prog) override {
        push_scope();

        ast::Replacer::visit(prog);

        pop_scope();
    }
    void visit(ast::GateDecl& decl) override {
        push_scope();
        for (auto& param : decl.q_params())
            set_var(param, {});

        ast::Replacer::visit(decl);

        pop_scope();
    }

    /* Overrides */
    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::RegisterDecl& decl) override {
        set_var(decl.id(), {decl.size()});
        return std::nullopt;
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::AncillaDecl& decl) override {
        set_var(decl.id(), {decl.size()});
        return std::nullopt;
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::BarrierGate& gate) override {
        if (auto num = repeats(gate.args())) {
            std::list<ast::ptr<ast::Gate>> ret;

            // Do the expansion
            for (int i = 0; i < *num; i++) {
                std::vector<ast::VarAccess> args;
                gate.foreach_arg([&args, this, i](auto& arg) {
                    args.emplace_back(expand(arg, i));
                });

                ret.emplace_back(std::make_unique<ast::BarrierGate>(
                    ast::BarrierGate(gate.pos(), std::move(args))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::CNOTGate& gate) override {
        if (auto num = repeats({gate.ctrl(), gate.tgt()})) {
            std::list<ast::ptr<ast::Gate>> ret;

            // Do the expansion
            for (int i = 0; i < *num; i++) {
                auto ctrl = expand(gate.ctrl(), i);
                auto tgt = expand(gate.tgt(), i);

                ret.emplace_back(std::make_unique<ast::CNOTGate>(ast::CNOTGate(
                    gate.pos(), std::move(ctrl), std::move(tgt))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::UGate& gate) override {
        if (auto num = repeats({gate.arg()})) {
            std::list<ast::ptr<ast::Gate>> ret;

            // Do the expansion
            for (int i = 0; i < *num; i++) {
                auto theta = ast::object::clone(gate.theta());
                auto phi = ast::object::clone(gate.phi());
                auto lambda = ast::object::clone(gate.lambda());
                auto arg = expand(gate.arg(), i);

                ret.emplace_back(std::make_unique<ast::UGate>(
                    ast::UGate(gate.pos(), std::move(theta), std::move(phi),
                               std::move(lambda), std::move(arg))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::list<ast::ptr<ast::Gate>>>
    replace(ast::DeclaredGate& gate) override {

        if (gate.name() == "rz") {

            std::list<ast::ptr<ast::Gate>> ret;
            std::vector<ast::ptr<ast::Expr>> c_args;
            std::vector<ast::VarAccess> q_args;
            gate.foreach_carg([&c_args, this](auto& arg) {
                c_args.emplace_back(ast::object::clone(arg));
            });
            gate.foreach_qarg([&q_args, this](auto& arg) {
                q_args.emplace_back(arg);
            });
            ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                ast::DeclaredGate(gate.pos(), "REPLACED_RZ",
                                  std::move(c_args), std::move(q_args))));
            
            return std::move(ret);
        }

        if (auto num = repeats(gate.qargs())) {
            std::list<ast::ptr<ast::Gate>> ret;

            // Do the expansion
            for (int i = 0; i < *num; i++) {
                std::vector<ast::ptr<ast::Expr>> c_args;
                std::vector<ast::VarAccess> q_args;
                gate.foreach_carg([&c_args, this](auto& arg) {
                    c_args.emplace_back(ast::object::clone(arg));
                });
                gate.foreach_qarg([&q_args, this, i](auto& arg) {
                    q_args.emplace_back(expand(arg, i));
                });

                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(gate.pos(), gate.name(),
                                      std::move(c_args), std::move(q_args))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::ResetStmt& stmt) override {
        if (auto num = repeats({stmt.arg()})) {
            std::list<ast::ptr<ast::Stmt>> ret;

            // Do the expansion
            for (int i = 0; i < *num; i++) {
                auto arg = expand(stmt.arg(), i);

                ret.emplace_back(std::make_unique<ast::ResetStmt>(
                    ast::ResetStmt(stmt.pos(), std::move(arg))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::list<ast::ptr<ast::Stmt>>>
    replace(ast::MeasureStmt& stmt) override {
        if (auto num = repeats({stmt.c_arg(), stmt.q_arg()})) {
            std::list<ast::ptr<ast::Stmt>> ret;

            // Do the expansion
            for (int i = 0; i < *num; i++) {
                auto q_arg = expand(stmt.q_arg(), i);
                auto c_arg = expand(stmt.c_arg(), i);

                ret.emplace_back(
                    std::make_unique<ast::MeasureStmt>(ast::MeasureStmt(
                        stmt.pos(), std::move(q_arg), std::move(c_arg))));
            }

            return std::move(ret);
        } else {
            return std::nullopt;
        }
    }

  private:
    using type_info = std::variant<std::monostate, int>;
    std::list<std::unordered_map<std::string, type_info>> symbol_table;

    void push_scope() { symbol_table.push_front({}); }

    void pop_scope() { symbol_table.pop_front(); }

    void set_var(std::string x, type_info t) { symbol_table.front()[x] = t; }

    std::optional<type_info> lookup(std::string x) {
        for (auto& table : symbol_table) {
            if (auto it = table.find(x); it != table.end()) {
                return std::make_optional(it->second);
            }
        }

        return std::nullopt;
    }

    // Compute the number of repeats with different offsets for a given list of
    // arguments
    std::optional<int> repeats(const std::vector<ast::VarAccess>& args) {
        std::optional<int> ret = std::nullopt;

        for (auto& arg : args) {
            auto ty = lookup(arg.var());
            if (std::holds_alternative<int>(*ty) && !arg.offset()) {
                auto size = std::get<int>(*ty);
                if (!ret) {
                    ret = size;
                } else if (size != *ret) {
                    std::cerr << "Warning: gate or measurement applied to "
                                 "different size registers\n";
                }
            }
        }

        return ret;
    }

    // Expand an argument with a given offset if register typed, otherwise copy
    // it
    ast::VarAccess expand(const ast::VarAccess& arg, int offset) {
        auto ty = lookup(arg.var());

        if (std::holds_alternative<int>(*ty) && !arg.offset())
            return ast::VarAccess(arg.pos(), arg.var(), offset);
        else
            return ast::VarAccess(arg);
    }

    // Debugging
    void print_current_scope() {
        for (auto& [x, ty] : symbol_table.front()) {
            if (std::holds_alternative<int>(ty)) {
                std::cout << x << ": Register(" << std::get<int>(ty) << "), ";
            } else {
                std::cout << x << ": Bit, ";
            }
        }
        std::cout << "\n";
    }
};

void desugar(ast::ASTNode& node) {
    DesugarImpl alg;
    alg.run(node);
}

} /* namespace transformations */
} /* namespace staq */

#endif /* TRANSFORMATIONS_SYNTH_HPP_ */
