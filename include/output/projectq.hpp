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
  std::unordered_map<std::string, std::string> qasmstd_to_projectq {
    { "id", "ops.Rz(0)" },
    { "x", "ops.X" },
    { "y", "ops.Y" },
    { "z", "ops.Z" },
    { "h", "ops.H" },
    { "s", "ops.S" },
    { "sdg", "ops.Sdag" },
    { "t", "ops.T" },
    { "tdg", "ops.Tdag" },
    { "cx", "ops.CNOT" },
    { "cz", "ops.CZ" },
    { "ccx", "ops.Toffoli" },
    { "rx", "ops.Rx" },
    { "ry", "ops.Ry" },
    { "rz", "ops.Rz" },
    { "u1", "ops.Rz" },
    { "crz", "ops.CRz" },
    { "cu1", "ops.CRz" } };

  class ProjectQOutputter final : public ast::Visitor {
  public:

    struct config {
      bool standalone = true;
      std::string circuit_name = "qasmcircuit";
    };

    ProjectQOutputter(std::ostream& os) : Visitor(), os_(os) {}
    ProjectQOutputter(std::ostream& os, const config& params) : Visitor(), os_(os), config_(params) {}
    ~ProjectQOutputter() = default;

    void run(ast::Program& prog) {
      prefix_ = "";
      ambiguous_ = false;
      ancillas_.clear();

      prog.accept(*this);
    }
      
    // Variables
    void visit(ast::VarAccess& ap) { os_ << ap; }

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
      os_ << expr.op();
      if (expr.op() == ast::UnaryOp::Neg) {
        auto tmp = ambiguous_;
        ambiguous_ = true;
        expr.subexp().accept(*this);
        ambiguous_ = tmp;
      } else {
        os_ << "(";
        expr.subexp().accept(*this);
        os_ << ")";
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
      // Hack because lambda is reserved by python
      if (expr.var() == "lambda")
        os_ << "lambd";
      else
        os_ << expr.var();
    }

    // Statements
    void visit(ast::MeasureStmt& stmt) {
      os_ << prefix_ << "Measure | " << stmt.q_arg() << "\t# " << stmt;
      os_ << prefix_ << stmt.c_arg() << " = int(" << stmt.q_arg() << ")\n";
    }

    void visit(ast::ResetStmt& stmt) {
      os_ << prefix_ << "reset(" << stmt.arg() << ")\t# " << stmt;
    }

    void visit(ast::IfStmt& stmt) {
      os_ << prefix_ << "if sum(v<<i for i, v in enumerate(" << stmt.var() << "[::-1])) == (";
      os_ << stmt.cond() << "%len(" << stmt.var() << "):";
      os_ << "# " << stmt;

      prefix_ += "    ";
      stmt.then().accept(*this);
      prefix_.resize(prefix_.size() - 4);
    }

    // Gates
    void visit(ast::UGate& gate) {
      os_ << prefix_ << "UGate(";
      gate.theta().accept(*this);
      os_ << ", ";
      gate.phi().accept(*this);
      os_ << ", ";
      gate.lambda().accept(*this);
      os_ << ") | ";
      gate.arg().accept(*this);
      os_ << "\t# " << gate;
    }

    void visit(ast::CNOTGate& gate) {
      os_ << prefix_ << "ops.CNOT | (";
      gate.ctrl().accept(*this);
      os_ << ", ";
      gate.tgt().accept(*this);
      os_ << ")\t#" << gate;
    }

    void visit(ast::BarrierGate& gate) {
      os_ << prefix_ << "ops.Barrier | (";
      for (int i = 0; i < gate.num_args(); i++) {
        if (i > 0)
          os_ << ", ";
        gate.arg(i).accept(*this);
      }
      os_ <<  ")\t# " << gate;
    }

    void visit(ast::DeclaredGate& gate) {
      os_ << prefix_;

      // Different syntax for built in gates & qasm circuits...
      // may be able to use decompositions to avoid this
      if (auto it = qasmstd_to_projectq.find(gate.name()); it != qasmstd_to_projectq.end()) {
        os_ << it->second << "(";

        for (int i = 0; i < gate.num_cargs(); i++) {
          if (i != 0)
            os_ << ", ";
          gate.carg(i).accept(*this);
        }
        os_ << ") | (";
        for (int i = 0; i < gate.num_qargs(); i++) {
          if (i > 0)
            os_ << ", ";
          gate.qarg(i).accept(*this);
        }
        os_ << ")";
      }
      else {
        os_ << gate.name() << "(";

        for (int i = 0; i < gate.num_cargs(); i++) {
          if (i != 0)
            os_ << ", ";
          gate.carg(i).accept(*this);
        }
        for (int i = 0; i < gate.num_qargs(); i++) {
          if (gate.num_cargs() > 0 || i > 0)
            os_ << ", ";
          gate.qarg(i).accept(*this);
        }
        os_ << ")";
      }

      os_ << "\t# " << gate;
    }

    // Declarations
    void visit(ast::GateDecl& decl) {
      if (decl.is_opaque())
        throw std::logic_error("Opaque declarations not supported");
      
      if (qasmstd_to_projectq.find(decl.id()) == qasmstd_to_projectq.end()) {
        os_ << prefix_ << "def " << decl.id() << "(";
        for (auto i = 0; i < decl.c_params().size(); i++) {
          if (i != 0)
            os_ << ", ";
          // Hack because lambda is reserved by python
          if (decl.c_params()[i] == "lambda")
            os_ << "lambd";
          else
            os_ << decl.c_params()[i];
        }

        for (auto i = 0; i < decl.q_params().size(); i++) {
          if (decl.c_params().size() > 0 || i != 0)
            os_ << ", ";
          os_ << " " << decl.q_params()[i];
        }
        os_ << ")";

        os_ << ":\t# " << "gate " << decl.id() << "\n";

        prefix_ += "    ";
        decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });

        // deallocate all ancillas
        for (auto& [name, size] : ancillas_) {
          for (int i = 0; i < size; i++) {
            os_ << prefix_ << eng_ << ".deallocate_qubit(" << name << "[" << i << "])\n";
          }
        }
        prefix_.resize(prefix_.size() - 4);
        os_ << "\n";
      }
    }

    void visit(ast::OracleDecl& decl) {
      throw std::logic_error("ProjectQ has no support for oracle declarations via logic files");
    }

    void visit(ast::RegisterDecl& decl) {
      if (decl.is_quantum()) {
        os_ << prefix_ << decl.id() << " = " << eng_ << ".allocate_qureg(" << decl.size() << ")";
      } else {
        os_ << prefix_ << decl.id() << " = [None] * " << decl.size();
      }
      os_ << "\t# " << decl;
    }

    void visit(ast::AncillaDecl& decl) {
      os_ << prefix_ << decl.id() << " = " << eng_ << ".allocate_qureg(" << decl.size() << ")";
      os_ << "\t# " << decl;
      ancillas_.push_back(std::make_pair(decl.id(), decl.size()));
    }
    
    // Program
    void visit(ast::Program& prog) {
      if (config_.standalone) {
        os_ << "from projectq import MainEngine, ops\n";
      }
      os_ << "from projectq import ops\n";
      os_ << "from math import pi,exp,sin,cos,tan,log as ln,sqrt\n";
      os_ << "import numpy as np\n\n";

      // QASM U gate
      os_ << "class UGate(ops.BasicGate):\n";
      os_ << "    def __init__(self, theta, phi, lambd):\n";
      os_ << "        ops.BasicGate.__init__(self)\n";
      os_ << "        self.theta = theta\n";
      os_ << "        self.phi = phi\n";
      os_ << "        self.lambd = lambd\n\n";
      os_ << "    def __str__(self):\n";
      os_ << "        return str(self.__class__.__name__) + \"(\" + str(self.theta) + \",\" \\\n";
      os_ << "               + str(self.phi) + \",\" + str(self.lambd) + \")\"\n\n";
      os_ << "    def tex_str(self):\n";
      os_ << "        return str(self.__class__.__name__) + \"$(\" + str(self.theta) + \",\" \\\n";
      os_ << "               + str(self.phi) + \",\" + str(self.lambd) + \")$\"\n\n";
      os_ << "    def get_inverse(self):\n";
      os_ << "        tmp = 2 * pi\n";
      os_ << "        return self.__class__(-self.theta + tmp, -self.lambd + tmp, -self.phi + tmp)\n\n";
      os_ << "    def __eq__(self, other):\n";
      os_ << "        if isinstance(other, self.__class__):\n";
      os_ << "            return self.theta == other.theta \\\n";
      os_ << "                   & self.phi == other.phi \\\n";
      os_ << "                   & self.lambd == other.lambd\n";
      os_ << "        else:\n";
      os_ << "            return False\n\n";
      os_ << "    def __ne__(self, other):\n";
      os_ << "        return not self.__eq__(other)\n\n";
      os_ << "    def __hash__(self):\n";
      os_ << "        return hash(str(self))\n\n";
      os_ << "    @property\n";
      os_ << "    def matrix(self):\n";
      os_ << "        return np.matrix([[exp(-1j*(self.phi+self.lambd)/2)*cos(self.theta/2),\n";
      os_ << "                           -exp(-1j*(self.phi-self.lambd)/2)*sin(self.theta/2)],\n";
      os_ << "                          [exp(1j*(self.phi-self.lambd)/2)*sin(self.theta/2),\n";
      os_ << "                           exp(1j*(self.phi+self.lambd)/2)*cos(self.theta/2)]])\n\n";

      // QASM reset statement
      os_ << "def reset(qubit):\n";
      os_ << "    ops.Measure | qubit\n";
      //os_ << "    qubit.eng.flush()\n";
      os_ << "    if int(qubit):\n";
      os_ << "        ops.X | qubit\n\n";


      os_ << "def " << config_.circuit_name << "(" << eng_ << "):\n";
      prefix_ = "    ";

      // Program body
      prog.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });

      os_ << "\n";
      prefix_ = "";

      // Standalone simulation
      if (config_.standalone) {
        os_ << "if __name__ == \"__main__\":\n";
        os_ << "    " << eng_ << " = MainEngine()\n";
        os_ << "    " << config_.circuit_name << "(" << eng_ << ")\n\n";
      }
    }

  private:
    std::ostream& os_;
    config config_;

    std::string prefix_ = "";
    std::string eng_ = "eng";
    std::list<std::pair<std::string, int> > ancillas_{};
    bool ambiguous_ = false;

  };

  void output_projectq(ast::Program& prog) {
    ProjectQOutputter outputter(std::cout);
    outputter.run(prog);
  }

  void write_projectq(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
      std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
      ProjectQOutputter outputter(ofs);
      outputter.run(prog);
    }

    ofs.close();
  }

}
}
