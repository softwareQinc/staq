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

#include "ast/ast.hpp"

namespace synthewareQ {
namespace output {

  /** \brief Equivalent quil standard gates for qasm standard gates */
  std::unordered_map<std::string, std::string> qasmstd_to_quilstd {
    { "id", "I" },
    { "x", "X" },
    { "y", "Y" },
    { "z", "Z" },
    { "h", "H" },
    { "s", "S" },
    { "sdg", "DAGGER S" },
    { "t", "T" },
    { "tdg", "DAGGER T" },
    { "cx", "CNOT" },
    { "ccx", "CCNOT" },
    { "rx", "RX" },
    { "ry", "RY" },
    { "rz", "RZ" },
    { "u1", "RZ" },
    { "crz", "CPHASE" },
    { "cu1", "CPHASE" } };

  class QuilOutputter final : public ast::Visitor {
  public:

    struct config {
      bool std_includes = false; // stdgates.quil is not supported natively by quilc
    };

    QuilOutputter(std::ostream& os) : Visitor(), os_(os) {}
    QuilOutputter(std::ostream& os, const config& params) : Visitor(), os_(os), config_(params) {}
    ~QuilOutputter() = default;

    void run(ast::Program& prog) {
      circuit_local_ = false;
      ambiguous_ = false;
      max_qbit_ = 0;
      max_cbit_ = 0;
      globals_.clear();

      prog.accept(*this);
    }
      
    // Variables
    void visit(ast::VarAccess& ap) {
      // Assumes the program has been fully desugared, and so plain variable
      // accesses refer to circuit parameters while dereferences refer to
      // qubit addresses
      if (ap.offset()) {
        auto [r, length] = globals_[ap.var()];
        os_ << r + *(ap.offset());
      } else {
        os_ << ap.var();
      }
    }

    // Expressions
    void visit(ast::BExpr& expr) {
      auto tmp = ambiguous_;
      ambiguous_ = true;
      if (tmp) {
        os_ << "(";
        expr.lexp().accept(*this);
        os_ << expr.op();
        expr.rexp().accept(*this);
        os_ << ")";
      } else {
        expr.lexp().accept(*this);
        os_ << expr.op();
        expr.rexp().accept(*this);
      }
      ambiguous_ = tmp;
    }
    
    void visit(ast::UExpr& expr) {
      switch(expr.op()) {
      case ast::UnaryOp::Neg: {
        auto tmp = ambiguous_;
        ambiguous_ = true;
        os_ << "-";
        expr.subexp().accept(*this);
        ambiguous_ = tmp;
        break;
      }
      case ast::UnaryOp::Sin:
        os_ << "sin(";
        expr.subexp().accept(*this);
        os_ << ")";
        break;
      case ast::UnaryOp::Cos:
        os_ << "cos(";
        expr.subexp().accept(*this);
        os_ << ")";
        break;
      case ast::UnaryOp::Tan:
        std::cerr << "Error: tan not supported by quil\n";
        break;
      case ast::UnaryOp::Ln:
        std::cerr << "Error: ln not supported by quil\n";
        break;
      case ast::UnaryOp::Sqrt:
        os_ << "sqrt(";
        expr.subexp().accept(*this);
        os_ << ")";
        break;
      case ast::UnaryOp::Exp:
        os_ << "exp(";
        expr.subexp().accept(*this);
        os_ << ")";
        break;
      default:
        break;
      }
    }

    void visit(ast::PiExpr&) {
      os_ << "pi";
    }

    void visit(ast::IntExpr& expr) {
      os_ << expr.value();
    }

    void visit(ast::RealExpr& expr) {
      os_ << expr.value();
    }

    void visit(ast::VarExpr& expr) {
      os_ << "%" << expr.var();
    }

    // Statements
    void visit(ast::MeasureStmt& stmt) {
      os_ << "MEASURE ";
      stmt.q_arg().accept(*this);
      os_ << " ";
      stmt.c_arg().accept(*this);
      os_ << "\t# " << stmt;
    }

    void visit(ast::ResetStmt& stmt) {
      os_ << "CLEAR ";
      stmt.arg().accept(*this);
      os_ << " [" << max_cbit_ << "]";
      os_ << "\t# " << stmt;
    }

    void visit(ast::IfStmt& stmt) {
      auto [r, length] = globals_[stmt.var()];

      // Checking each bit separately avoids allocating
      // a garbage cbit for the result of an EQ. That
      // and the EQ instruction is not well-documented
      auto tmp = stmt.cond();
      for (auto i = 0; i < length; i++) {
        if (tmp % 2 == 1)
          os_ << "JUMP-UNLESS";
        else
          os_ << "JUMP-WHEN";

        os_ << " @end" << stmt.uid() << " [" << r + i << "]";
        os_ << "\t# " << stmt;
      }

      stmt.then().accept(*this);
      
      os_ << "LABEL @end" << stmt.uid() << "\n";
    }

    // Gates
    void visit(ast::UGate& gate) {
      if (circuit_local_)
        os_ << "    ";

      os_ << "U(";
      gate.theta().accept(*this);
      os_ << ", ";
      gate.phi().accept(*this);
      os_ << ", ";
      gate.lambda().accept(*this);
      os_ << ")";

      os_ << " ";
      gate.arg().accept(*this);
      os_ << "\t# " << gate;
    }

    void visit(ast::CNOTGate& gate) {
      if (circuit_local_)
        os_ << "    ";

      os_ << "CNOT ";
      gate.ctrl().accept(*this);
      os_ << " ";
      gate.tgt().accept(*this);
      os_ << "\t# " << gate;
    }

    void visit(ast::BarrierGate& gate) {
      if (circuit_local_)
        os_ << "    ";

      // An approximation to openQASM's barrier
      os_ << "PRAGMA parallelization_barrier";
      os_ << "\t# " << gate;
    }

    void visit(ast::DeclaredGate& gate) {
      if (circuit_local_)
        os_ << "    ";

      if (auto it = qasmstd_to_quilstd.find(gate.name()); it != qasmstd_to_quilstd.end())
        os_ << it->second;
      else 
        os_ << gate.name();

      if (gate.num_cargs() > 0) {
        os_ << "(";
        for (int i = 0; i < gate.num_cargs(); i++) {
          if (i != 0)
            os_ << ", ";
          gate.carg(i).accept(*this);
        }
        os_ << ")";
      }

      for (int i = 0; i < gate.num_qargs(); i++) {
        os_ << " ";
        gate.qarg(i).accept(*this);
      }

      os_ << "\t# " << gate;
    }

    // Declarations
    void visit(ast::GateDecl& decl) {
      if (decl.is_opaque())
        throw std::logic_error("Quil instruction set has no support for opaque declarations");
      
      if (config_.std_includes && qasmstd_to_quilstd.find(decl.id()) != qasmstd_to_quilstd.end()) {
        os_ << "DEFCIRCUIT " << decl.id();

        if (decl.c_params().size() > 0) {
          os_ << "(";
          for (auto i = 0; i < decl.c_params().size(); i++) {
            if (i != 0)
              os_ << ", ";
            os_ << "%" << decl.c_params()[i];
          }
          os_ << ")";
        }

        for (auto i = 0; i < decl.q_params().size(); i++) {
          os_ << " " << decl.q_params()[i];
        }

        os_ << ":\t# " << "gate " << decl.id() << "\n";

        circuit_local_ = true;
        decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
        circuit_local_ = false;
        os_ << "\n";
      }
    }

    void visit(ast::OracleDecl& decl) {
      throw std::logic_error("Quil instruction set has no support for oracle declarations");
    }

    void visit(ast::RegisterDecl& decl) {
      if (decl.is_quantum()) {
        globals_[decl.id()] = std::make_pair(max_qbit_, decl.size());
        max_qbit_ += decl.size();
      } else {
        globals_[decl.id()] = std::make_pair(max_cbit_, decl.size());
        max_cbit_ += decl.size();
      }
      os_ << "# " << decl;
    }

    void visit(ast::AncillaDecl& decl) {
      throw std::logic_error("Quil instruction set has no support for local ancillas");
    }
    
    // Program
    void visit(ast::Program& prog) {
      if (!config_.std_includes) {
        os_ << "DEFGATE X:\n";
        os_ << "    0, 1\n";
        os_ << "    1, 0\n\n";
        os_ << "DEFGATE CNOT:\n";
        os_ << "    1, 0, 0, 0\n";
        os_ << "    0, 1, 0, 0\n";
        os_ << "    0, 0, 0, 1\n";
        os_ << "    0, 0, 1, 0\n\n";
      } else {
        os_ << "INCLUDE \"stdgates.quil\"\n\n";
      }

      // QASM U gate
      os_ << "DEFGATE U(%theta, %phi, %lambda):\n";
      os_ << "    EXP(-i*(%phi+%lambda)/2)*COS(%theta/2), -EXP(-i*(%phi-%lambda)/2)*SIN(%theta/2)\n";
      os_ << "    EXP(i*(%phi-%lambda)/2)*SIN(%theta/2), EXP(i*(%phi+%lambda)/2)*COS(%theta/2)\n\n";

      // QASM reset statement
      os_ << "DEFCIRCUIT CLEAR q scratch_bit:\n";
      os_ << "    MEASURE q scratch_bit\n";
      os_ << "    JUMP-UNLESS @end scratch_bit\n";
      os_ << "    X q\n";
      os_ << "    LABEL @end\n\n";

      // Program body
      prog.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
    }

  private:
    std::ostream& os_;
    config config_;

    bool circuit_local_ = false;
    bool ambiguous_ = false;
    int max_qbit_ = 0;
    int max_cbit_ = 0;
    std::unordered_map<ast::symbol, std::pair<int, int> > globals_{};

  };

  void output_quil(ast::Program& prog) {
    QuilOutputter outputter(std::cout);
    outputter.run(prog);
  }

  void write_quil(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
      std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
      QuilOutputter outputter(ofs);
      outputter.run(prog);
    }

    ofs.close();
  }

}
}
