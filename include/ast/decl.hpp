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
 * \file ast/decl.hpp
 * \brief openQASM declarations
 */
#pragma once

#include "stmt.hpp"

#include <list>

namespace synthewareQ {
namespace ast {

  static const std::set<std::string_view> qelib_defs {
    "u3", "u2", "u1", "cx", "id", "u0", "x", "y", "z",
    "h", "s", "sdg", "t", "tdg", "rx", "ry", "rz",
    "cz", "cy", "swap", "ch", "ccx", "crz", "cu1",
    "cu3"
  };

  /**
   * \class synthewareQ::ast::Decl
   * \brief Base class for openQASM declarations
   *
   * Declarations are attribute classes as they can occur in different
   * statement contexts. To avoid diamond inheritance, any derived declaration
   * should also inherit from a statement class
   */
  class Decl {
  protected:
    symbol id_;

  public:
    Decl(symbol id) : id_(id) {}
    virtual ~Decl() = default;

    const symbol& id() { return id_; }
    //virtual Type get_type() = 0;
  };

  /**
   * \class synthewareQ::ast::GateDecl
   * \brief Class for gate declarations
   * \see synthewareQ::ast::Decl
   */
  class GateDecl final : public Stmt, public Decl {
    bool opaque_;                  ///< whether the declaration is opaque
    std::vector<symbol> c_params_; ///< classical parameters
    std::vector<symbol> q_params_; ///< quantum parameters
    std::list<ptr<Gate> > body_;   ///< gate body

  public:
    /**
     * \brief Constructs a gate declaration
     *
     * \param pos The source position
     * \param id The gate identifier
     * \param c_params List of classical parameters
     * \param q_params List of quantum parameters
     * \param body List of gate statements
     */
    GateDecl(parser::Position pos, symbol id, bool opaque, std::vector<symbol> c_params,
             std::vector<symbol> q_params, std::list<ptr<Gate> >&& body)
      : Stmt(pos), Decl(id)
      , opaque_(opaque)
      , c_params_(c_params)
      , q_params_(q_params)
      , body_(std::move(body))
    {}

    bool is_opaque() { return opaque_; }
    std::vector<symbol>& c_params() { return c_params_; }
    std::vector<symbol>& q_params() { return q_params_; }
    std::list<ptr<Gate> >& body() { return body_; }
    void foreach_stmt(std::function<void(Gate&)> f) {
      for (auto it = body_.begin(); it != body_.end(); it++) f(**it);
    }

    std::list<ptr<Gate> >::iterator begin() { return body_.begin(); }
    std::list<ptr<Gate> >::iterator end() { return body_.end(); }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool suppress_std) const override {
      if (suppress_std && qelib_defs.find(id_) != qelib_defs.end())
        return os;
      
      os << (opaque_ ? "opaque " : "gate ") << id_;
      if (c_params_.size() > 0) {
        os << "(";
        for (auto it = c_params_.begin(); it != c_params_.end(); it++) {
          os << (it == c_params_.begin() ? "" : ",") << *it;
        }
        os << ")";
      }
      os << " ";
      for (auto it = q_params_.begin(); it != q_params_.end(); it++) {
        os << (it == q_params_.begin() ? "" : ",") << *it;
      }
      if (opaque_) {
        os << ";\n";
      } else {
        os << " {\n";
        for (auto it = body_.begin(); it != body_.end(); it++) {
          os << "\t" << **it;
        }
        os << "}\n";
      }
	  return os;
    }
    GateDecl* clone() const override {
      std::list<ptr<Gate> > tmp;
      for (auto it = body_.begin(); it != body_.end(); it++) {
        tmp.emplace_back(ptr<Gate>((*it)->clone()));
      }
      return new GateDecl(pos_, id_, opaque_, c_params_, q_params_, std::move(tmp));
    }
  };

  /**
   * \class synthewareQ::ast::OracleDecl
   * \brief Class for oracle declarations
   * \see synthewareQ::ast::Decl
   */
  class OracleDecl final : public Stmt, public Decl {
    std::vector<symbol> params_; ///< quantum parameters
    symbol fname_;               ///< filename of external declaration

  public:
    OracleDecl(parser::Position pos, symbol id, std::vector<symbol> params, symbol fname)
      : Stmt(pos), Decl(id)
      , params_(params)
      , fname_(fname)
    {}

    std::vector<symbol>& params() { return params_; }
    const symbol& fname() { return fname_; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << "oracle " << id_ << " ";
      for (auto it = params_.begin(); it != params_.end(); it++) {
        os << (it == params_.begin() ? "" : ",") << *it;
      }
      os << " { \"" << fname_ << "\" }\n";
	  return os;
    }
    OracleDecl* clone() const override {
      return new OracleDecl(pos_, id_, params_, fname_);
    }
  };

  /**
   * \class synthewareQ::ast::RegisterDecl
   * \brief Class for register declarations
   * \see synthewareQ::ast::Decl
   */
  class RegisterDecl final : public Stmt, public Decl {
    bool quantum_; ///< whether the register is quantum
    int size_;   ///< the size of the register

  public:
    /**
     * \brief Constructs a register declaration
     *
     * \param pos The source position
     * \param id The register identifier
     * \param quantum whether the register is a quantum register
     * \param size the size of the register
     */
    RegisterDecl(parser::Position pos, symbol id, bool quantum, int size)
      : Stmt(pos), Decl(id), quantum_(quantum), size_(size) {}

    bool is_quantum() { return quantum_; }
    int size() { return size_; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      os << (quantum_ ? "qreg " : "creg ") << id_ << "[" << size_ << "];\n";
      return os;
    }
    RegisterDecl* clone() const override {
      return new RegisterDecl(pos_, id_, quantum_, size_);
    }
  };
  
  /**
   * \class synthewareQ::ast::AncillaDecl
   * \brief Class for local register declarations
   * \see synthewareQ::ast::Decl
   */
  class AncillaDecl final : public Gate, public Decl {
    bool dirty_; ///< whether the register can be dirty
    int size_;   ///< the size of the register

  public:
    /**
     * \brief Constructs a register declaration
     *
     * \param pos The source position
     * \param id The register identifier
     * \param quantum whether the register is a quantum register
     * \param size the size of the register
     */
    AncillaDecl(parser::Position pos, symbol id, bool dirty, int size)
      : Gate(pos), Decl(id), dirty_(dirty), size_(size) {}

    bool is_dirty() { return dirty_; }
    int size() { return size_; }

    void accept(Visitor& visitor) override { visitor.visit(*this); }
    std::ostream& pretty_print(std::ostream& os, bool) const override {
      if (dirty_) os << "dirty ";
      os << "ancilla " << id_ << "[" << size_ << "];\n";
      return os;
    }
    AncillaDecl* clone() const override {
      return new AncillaDecl(pos_, id_, dirty_, size_);
    }
  };

}
}
