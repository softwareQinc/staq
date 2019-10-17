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
 * \file ast/ast.hpp
 * \brief openQASM syntax trees
 */
#pragma once

#include "parser/position.hpp"
#include "visitor.hpp"

#include <set>
#include <memory>

namespace synthewareQ {
namespace ast {

  template <typename T>
  using ptr = std::unique_ptr<T>;

  using symbol = std::string;

  /**
   * \class synthewareQ::ast::ASTNode
   * \brief Base class for AST nodes
   */
  class ASTNode {
    static int& max_uid_() { static int v; return v; }

  protected:
    const int uid_;
    const parser::Position pos_;

  public:
    ASTNode(parser::Position pos) : uid_(++max_uid_()), pos_(pos) { }
    virtual ~ASTNode() = default;

    int uid() const { return uid_; }
    parser::Position pos() const { return pos_; }

    virtual void accept(Visitor& visitor) = 0;
    virtual std::ostream& pretty_print(std::ostream& os) const = 0;
    virtual ASTNode* clone() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const ASTNode& node) {
      return node.pretty_print(os);
    }
  };


}
}
