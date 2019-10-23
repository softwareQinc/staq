/*
 * This file is part of synthewareQ.
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

#pragma once

#include "ast/replacer.hpp"
#include "ast/traversal.hpp"
#include "transformations/substitution.hpp"
#include "mapping/device.hpp"

#include <unordered_map>

namespace synthewareQ {
namespace mapping {

  /** \brief Applies a layout to a circuit */
  class LayoutTransformer final : public ast::Replacer {
  public:
    struct config {
      std::string register_name = "q";
    };

    LayoutTransformer() = default;
    LayoutTransformer(const config& params) : Replacer(), config_(params) {}
    ~LayoutTransformer() = default;

    void run(ast::Program& prog, const layout& l) {
      // Visit entire program, removing register declarations, then
      // add the physical register & apply substitutions
      prog.accept(*this);

      // Physical register declaration
      prog.body().emplace_front(std::make_unique<ast::RegisterDecl>(ast::RegisterDecl(
        prog.pos(), config_.register_name, true, l.size())));

      // Substitution
      std::unordered_map<ast::VarAccess, ast::VarAccess> subst;
      for (auto const& [access, idx] : l) {
        subst.insert({access, ast::VarAccess(parser::Position(), config_.register_name, idx)});
      }
      transformations::subst_ap_ap(subst, prog);
    }

    std::optional<std::list<ast::ptr<ast::Stmt> > > replace(ast::RegisterDecl& decl) override {
      if (decl.is_quantum()) return std::list<ast::ptr<ast::Stmt> >();
      else return std::nullopt;
    }

  private:
    config config_;
  };

  /** \brief A basic qubit layout algorithm */
  class BasicLayout final : public ast::PostVisitor {
  public:
    BasicLayout(Device& device) : PostVisitor(), device_(device) {}
    ~BasicLayout() = default;

    layout generate(ast::Program& prog) {
      current_ = layout();
      n_ = 0;

      prog.accept(*this);

      return current_;
    }

    void visit(ast::RegisterDecl& decl) override {
      if (decl.is_quantum()) {
        if (n_ + decl.size() <= device_.qubits_) {
          for (auto i = 0; i < decl.size(); i++) {
            current_[ast::VarAccess(parser::Position(), decl.id(), i)] = n_ + i;
          }
          n_ += decl.size();
        } else {
          std::cerr << "Error: can't fit program onto device " << device_.name_ << "\n";
        }
      }
    }

  private:
    Device device_;
    layout current_;
    size_t n_;
  };

  inline void apply_layout(const layout& l, ast::Program& prog) {
    LayoutTransformer alg;
    alg.run(prog, l);
  }

  inline layout compute_basic_layout(Device& device, ast::Program& prog) {
    BasicLayout gen(device);
    return gen.generate(prog);
  }

}
}
