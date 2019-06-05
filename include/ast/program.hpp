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

/**
 * \file ast/program.hpp
 * \brief openQASM programs
 */
#pragma once

#include "decl.hpp"

namespace synthewareQ {
namespace ast {

  /**
   * \class synthewareQ::ast::Program
   * \brief Program class
   */
  class Program : public ASTNode {
    bool std_include_;           ///< whether the program includes qelib1
    std::list<ptr<Stmt> > body_; ///< the body of the program

  public:
    Program(parser::Position pos, bool std_include, std::list<ptr<Stmt> >&& body)
      : ASTNode(pos)
      , std_include_(std_include)
      , body_(std::move(body))
    {}

    std::list<ptr<Stmt> >& body() { return body_; }
    void foreach_stmt(std::function<void(Stmt&)> f) {
      for (auto it = body_.begin(); it != body_.end(); it++) f(**it);
    }

    std::list<ptr<Stmt> >::iterator begin() { return body_.begin(); }
    std::list<ptr<Stmt> >::iterator end() { return body_.end(); }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os) const override {
      os << "OPENQASM 2.0;\n";
      if (std_include_) os << "include \"qelib1.inc\";\n";
      os << "\n";
      for (auto it = body_.begin(); it != body_.end(); it++) {
        (*it)->pretty_print(os, std_include_);
      }

      return os;
    }
    Program* clone() const override {
      std::list<ptr<Stmt> > tmp;
      for (auto it = body_.begin(); it != body_.end(); it++) {
        tmp.emplace_back(ptr<Stmt>((*it)->clone()));
      }
      return new Program(pos_, std_include_, std::move(tmp));
    }

  };

}
}
