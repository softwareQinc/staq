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
 * \file ast/var.hpp
 * \brief openQASM variable utilities
 */
#pragma once

#include "base.hpp"

#include <optional>

namespace synthewareQ {
namespace ast {

  /**
   * \class synthewareQ::ast::VarAccess
   * \brief Class for variable accesses
   */
  class VarAccess final : public ASTNode {
    symbol var_;                ///< the identifier
    std::optional<int> offset_; ///< optional offset into a register variable

  public:
    friend std::hash<VarAccess>;
    
    VarAccess(parser::Position pos, symbol var, std::optional<int> offset = std::nullopt)
      : ASTNode(pos)
      , var_(var)
      , offset_(offset)
    {}
    VarAccess(const VarAccess& va) : ASTNode(va.pos_), var_(va.var_), offset_(va.offset_) {}
    
    const symbol& var() const { return var_; }
    std::optional<int> offset() const { return offset_; }

    VarAccess& operator=(const VarAccess& v) {
      var_ = v.var_;
      offset_ = v.offset_;
      return *this;
    }

    bool operator==(const VarAccess& v) const {
      return var_ == v.var_ && offset_ == v.offset_;
    }
    bool operator<(const VarAccess& v) const {
      if (var_ == v.var_)
        return offset_ < v.offset_;
      else
        return var_ < v.var_;
    }
    bool contains(const VarAccess& v) const {
      if (offset_)
        return *this == v;
      else
        return v.var_ == var_;
    }

    friend std::size_t hash_value(const VarAccess& v) {
      size_t lhs = std::hash<symbol>{}(v.var_);
      lhs ^= std::hash<std::optional<int> >{}(v.offset_) + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
      return lhs;
    }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os) const override {
      os << var_;
      if (offset_) os << "[" << *offset_ << "]";
      return os;
    }
    VarAccess* clone() const override {
      return new VarAccess(pos_, var_, offset_);
    }
  };


}
}

namespace std {
  template<>
  struct hash<synthewareQ::ast::VarAccess> {
    size_t operator()(const synthewareQ::ast::VarAccess& v) const {
      size_t lhs = std::hash<std::string>{}(v.var_);
      lhs ^= std::hash<std::optional<int> >{}(v.offset_) + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
      return lhs;
      //return synthewareQ::ast::hash_value(var);
    }
  };
}
