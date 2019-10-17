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

#include "ast.hpp"
#include "visitor.hpp"

#include <unordered_map>

/**
 * \file ast/semantic.hpp
 * \brief Semantic analysis for syntax trees
 */
#pragma once

namespace synthewareQ {
namespace ast {

  class SemanticError : public std::exception {
  public:
    SemanticError() noexcept = default;
    ~SemanticError() {}
    const char* what() const noexcept {
      return "Error(s) occurred";
    }
  };

  /* Types */
  enum class BitType { Cbit, Qubit };
  struct GateType {
    int num_c_params;
    int num_q_params;
  };
  struct RegisterType {
    BitType type;
    int length;
  };
  struct RealType {};

  using Type = std::variant<BitType, GateType, RegisterType, RealType>;

  /** 
   * \brief Implementation of the semantic analysis compiler phase
   *  
   * Checks for anything that could cause a run-time error -- notably, 
   * type errors, invalid uniform gates, etc.
   */
  class SemanticChecker final : public Visitor {
  public:

    bool run(Program& prog) {
      prog.accept(*this);
      return error_;
    }

    void visit(VarAccess&) {}

    void visit(BExpr& expr) {
      expr.lexp().accept(*this);
      expr.rexp().accept(*this);
    }
    void visit(UExpr& expr) {
      expr.subexp().accept(*this);
    }
    void visit(PiExpr&) {}
    void visit(IntExpr&) {}
    void visit(RealExpr&) {}
    void visit(VarExpr& expr) {
      auto entry = lookup(expr.var());
      
      if (!entry) {
        std::cerr << expr.pos() << ": Identifier \"" << expr.var() << "\" undeclared\n";
        error_ = true;
      } else if (!std::holds_alternative<RealType>(*entry)) {
        std::cerr << expr.pos() << ": Identifier \"" << expr.var();
        std::cerr << "\" does not have numeric type\n";
        error_ = true;
      }
    }

    void visit(MeasureStmt& stmt) {
      check_uniform({stmt.q_arg(), stmt.c_arg()}, {BitType::Qubit, BitType::Cbit});
    }
    void visit(ResetStmt& stmt) {
      check_uniform({stmt.arg()}, {BitType::Qubit});
    }
    void visit(IfStmt& stmt) {
      auto entry = lookup(stmt.var());
      
      if (!entry) {
        std::cerr << stmt.pos() << ": Identifier \"" << stmt.var() << "\" undeclared\n";
        error_ = true;
      } else if (!std::holds_alternative<RegisterType>(*entry) ||
                 !(std::get<RegisterType>(*entry).type == BitType::Cbit)) {
        std::cerr << stmt.pos() << ": Identifier \"" << stmt.var();
        std::cerr << "\" does not have classical register type\n";
        error_ = true;
      } else {
        stmt.then().accept(*this);
      }
    }

    void visit(UGate& gate) {
      gate.theta().accept(*this);
      gate.phi().accept(*this);
      gate.lambda().accept(*this);

      check_uniform({gate.arg()}, {BitType::Qubit});
    }
    void visit(CNOTGate& gate) {
      check_uniform({gate.ctrl(), gate.tgt()}, {BitType::Qubit, BitType::Qubit});
    }
    void visit(BarrierGate& gate) {
      std::vector<std::optional<BitType> > types(gate.args().size(), std::nullopt);
      check_uniform(gate.args(), types);
    }
    void visit(DeclaredGate& gate) {
      auto entry = lookup_gate(gate.name());
      
      if (!entry) {
        std::cerr << gate.pos() << ": Gate \"" << gate.name() << "\" undeclared\n";
        error_ = true;
      } else {
        auto ty = *entry;
        if (ty.num_c_params != gate.num_cargs()) {
          std::cerr << gate.pos() << ": Gate \"" << gate.name() << "\" expects " << ty.num_c_params;
          std::cerr << " classical arguments, got " << gate.num_cargs() << "\n";
          error_ = true;
        } else if (ty.num_q_params != gate.num_qargs()) {
          std::cerr << gate.pos() << ": Gate \"" << gate.name() << "\" expects " << ty.num_q_params;
          std::cerr << " quantum arguments, got " << gate.num_qargs() << "\n";
          error_ = true;
        } else {
          gate.foreach_carg([this](Expr& expr){ expr.accept(*this); });

          std::vector<std::optional<BitType> > types(ty.num_q_params, BitType::Qubit);
          check_uniform(gate.qargs(), types);
        }
      }
    }

    void visit(GateDecl& decl) {
      if (lookup_gate(decl.id())) {
        std::cerr << decl.pos() << ": Gate \"" << decl.id() << "\" previous declared\n";
        error_ = true;
      } else {
        // Check the body
        push_scope();
        for (const ast::symbol& param : decl.c_params()) {
          set(param, RealType{});
        }
        for (const ast::symbol& param : decl.q_params()) {
          set(param, BitType::Qubit);
        }

        decl.foreach_stmt([this](Gate& gate){ gate.accept(*this); });

        pop_scope();

        // Add declaration
        set_gate(decl.id(), GateType{(int)decl.c_params().size(), (int)decl.q_params().size()});
      }
    }
    void visit(OracleDecl& decl) {
      if (lookup(decl.id())) {
        std::cerr << decl.pos() << ": Identifier \"" << decl.id() << "\" previous declared\n";
        error_ = true;
      } else {
        set_gate(decl.id(), GateType{ 0, (int)decl.params().size() });
      }
    }
    void visit(RegisterDecl& decl) {
      if (lookup(decl.id())) {
        std::cerr << decl.pos() << ": Identifier \"" << decl.id() << "\" previous declared\n";
        error_ = true;
      } else if (decl.size() < 0) {
        std::cerr << decl.pos() << ": Registers must have non-negative size\n";
        error_ = true;
      } else {
        set(decl.id(), RegisterType{decl.is_quantum() ? BitType::Qubit : BitType::Cbit, decl.size()});
      }
    }
    void visit(AncillaDecl& decl) {
      if (lookup(decl.id())) {
        std::cerr << decl.pos() << ": Identifier \"" << decl.id() << "\" previous declared\n";
        error_ = true;
      } else if (decl.size() < 0) {
        std::cerr << decl.pos() << ": Registers must have non-negative size\n";
        error_ = true;
      } else {
        set(decl.id(), RegisterType{BitType::Qubit, decl.size()});
      }
    }

    void visit(Program& prog) {
      push_scope();

      prog.foreach_stmt([this](Stmt& stmt){ stmt.accept(*this); });

      pop_scope();
    }

  private:
    bool error_ = false;
    std::unordered_map<ast::symbol, GateType> gate_decls_{};
    std::list<std::unordered_map<ast::symbol, Type> > symbol_table_{{}};

    void push_scope() { symbol_table_.push_front({}); }
    void pop_scope() { symbol_table_.pop_front(); }
    std::optional<Type> lookup(const ast::symbol& id) {
      for (auto& table : symbol_table_) {
        if (auto it = table.find(id); it != table.end()) {
          return it->second;
        }
      }
      return std::nullopt;
    }
    void set(const ast::symbol& id, Type typ) {
      if (symbol_table_.empty())
        throw std::logic_error("No current symbol table!");

      symbol_table_.front()[id] = typ;
    }

    std::optional<GateType> lookup_gate(const ast::symbol& id) {
      if (auto it = gate_decls_.find(id); it != gate_decls_.end()) {
          return it->second;
      }
      return std::nullopt;
    }
    void set_gate(const ast::symbol& id, GateType typ) {
      gate_decls_[id] = typ;
    }

    /** \brief Checks a vector of bit accesses */
    void check_uniform(const std::vector<VarAccess>& args, const std::vector<std::optional<BitType> >& types) {
      int mapping_size = -1;

      for (auto i = 0; i < args.size(); i++) {
        auto entry = lookup(args[i].var());
      
        if (!entry) {
          std::cerr << args[i].pos() << ": Identifier \"" << args[i].var() << "\" undeclared\n";
          error_ = true;
        } else if (std::holds_alternative<BitType>(*entry)) {
          auto ty = std::get<BitType>(*entry);

          if (args[i].offset()) { // Check that the bit is not dereferenced
            std::cerr << args[i].pos() << ": Attempting to dereference bit type\n";
            error_ = true;
          } else if (types[i] && ty != *(types[i])) { // Check if it's compatible with the type list
            std::cerr << args[i].pos() << ": Bit is of wrong type\n";
            error_ = true;
          }
        } else if (std::holds_alternative<RegisterType>(*entry) && args[i].offset()) {
          auto ty = std::get<RegisterType>(*entry);

          if (0 > *(args[i].offset()) || *(args[i].offset()) >= ty.length) { // Check that it's within bounds
            std::cerr << args[i].pos() << ": Bit access out of bounds\n";
            error_ = true;
          } else if (types[i] && ty.type != *(types[i])) { // Check if it's compatible with the type list
            std::cerr << args[i].pos() << ": Bit is of wrong type\n";
            error_ = true;
          }
        } else if (std::holds_alternative<RegisterType>(*entry)) {
          auto ty = std::get<RegisterType>(*entry);

          if (mapping_size == -1) {
            mapping_size = ty.length;
          } else if (mapping_size != ty.length) { // Check that the register length is consistent
            std::cerr << args[i].pos() << ": Register has incompatible length\n";
            error_ = true;
          }

          if (types[i] && ty.type != *(types[i])) { // Check if it's compatible with the type list
            std::cerr << args[i].pos() << ": Register is of wrong type\n";
            error_ = true;
          }
        } else {
          std::cerr << args[i].pos() << ": Identifier is not a bit or register\n";
          error_ = true;
        }
      }
    }
        
  };

  inline void check_source(Program& prog) {
    SemanticChecker analysis;
    if (analysis.run(prog))
      throw SemanticError();
  }
      

}
}
