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

#include <typeinfo>

namespace synthewareQ {
namespace output {

  /** \brief Equivalent projectQ standard gates for qasm standard gates */
  std::unordered_map<std::string, std::string> qasmstd_to_projectq {
    { "id", "ops.Rz(0)" },
    { "x", "ops.XGate" },
    { "y", "ops.YGate" },
    { "z", "ops.ZGate" },
    { "h", "ops.HGate" },
    { "s", "ops.SGate" },
    { "sdg", "SdagGate" },
    { "t", "ops.TGate" },
    { "tdg", "TdagGate" },
    { "cx", "CNOTGate" },
    { "cz", "CZGate" },
    { "ccx", "CCXGate" },
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
      switch(expr.op()) {
      case ast::UnaryOp::Neg: {
        auto tmp = ambiguous_;
        ambiguous_ = true;
        os_ << "-";
        expr.subexp().accept(*this);
        ambiguous_ = tmp;
        break;
      }
      default:
        os_ << expr.op();
        os_ << "(";
        expr.subexp().accept(*this);
        os_ << ")";
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
      os_ << sanitize(expr.var());
    }

    // Statements
    void visit(ast::MeasureStmt& stmt) {
      os_ << prefix_ << "ops.Measure | " << stmt.q_arg() << "\n";
      os_ << prefix_ << stmt.c_arg() << " = int(" << stmt.q_arg() << ")\n";
    }

    void visit(ast::ResetStmt& stmt) {
      os_ << prefix_ << "Reset | " << stmt.arg() << "\n";
    }

    void visit(ast::IfStmt& stmt) {
      os_ << prefix_ << "if sum(v<<i for i, v in enumerate(" << stmt.var();
      os_ << "[::-1])) == " << stmt.cond() << ":\n";

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
      os_ << "\n";
    }

    void visit(ast::CNOTGate& gate) {
      os_ << prefix_ << "ops.CNOT | (";
      gate.ctrl().accept(*this);
      os_ << ", ";
      gate.tgt().accept(*this);
      os_ << ")\n";
    }

    void visit(ast::BarrierGate& gate) {
      os_ << prefix_ << "ops.Barrier | (";
      for (int i = 0; i < gate.num_args(); i++) {
        if (i > 0)
          os_ << ", ";
        gate.arg(i).accept(*this);
      }
      os_ <<  ")\n";
    }

    void visit(ast::DeclaredGate& gate) {
      os_ << prefix_;

      if (auto it = qasmstd_to_projectq.find(gate.name()); it != qasmstd_to_projectq.end())
        os_ << it->second << "(";
      else
        os_ << gate.name() << "(";

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
      os_ << ")\n";
    }

    // Declarations
    void visit(ast::GateDecl& decl) {
      if (decl.is_opaque())
        throw std::logic_error("Opaque declarations not supported");
      
      if (qasmstd_to_projectq.find(decl.id()) == qasmstd_to_projectq.end()) {

        os_ << "class " << decl.id() << "(ops.BasicGate):\n";

        // Class instantiation
        os_ << "    def __init__(self, ";
        for (auto i = 0; i < decl.c_params().size(); i++) {
          if (i != 0)
            os_ << ", ";
          os_ << sanitize(decl.c_params()[i]);
        }
        os_ << "):\n";
        os_ << "        ops.BasicGate.__init__(self)\n";
        for (auto param : decl.c_params()) {
          auto tmp = sanitize(param);
          os_ << "        self." << tmp << " = " << tmp << "\n";
        }
        os_ << "\n";

        // String representation
        os_ << "    def __str__(self):\n";
        os_ << "        return str(self.__class__.__name__) + \"(\" + ";
        for (auto i = 0; i < decl.c_params().size(); i++) {
          if (i != 0)
            os_ << " + \",\" + ";
          os_ << "str(self." << sanitize(decl.c_params()[i]) << ")";
        }
        os_ << " + \")\"\n\n";

        // Equality 
        os_ << "    def __eq__(self, other):\n";
        os_ << "        if isinstance(other, self.__class__):\n";
        os_ << "            return True";
        for (auto i = 0; i < decl.c_params().size(); i++) {
          auto tmp = sanitize(decl.c_params()[i]);
          os_ << "\\\n                   & self." << tmp << " == other." << tmp;
        }
        os_ << "\n";
        os_ << "        else:\n";
        os_ << "            return False\n\n";

        os_ << "    def __ne__(self, other):\n";
        os_ << "        return not self.__eq__(other)\n\n";

        // Hashing
        os_ << "    def __hash__(self):\n";
        os_ << "        return hash(str(self))\n\n";

        // Implementation as an override of ||
        os_ << "    def __or__(self, qubits):\n";
        auto tmp = eng_;
        eng_ = "qubits[0].engine";

        os_ << "        if len(qubits) != " << decl.q_params().size() << ":\n";
        os_ << "            raise TypeError(\"Expected " << decl.q_params().size()
                                                         << " qubits, given \" + len(qubits))\n";
        for (auto i = 0; i < decl.q_params().size(); i++) {
          os_ << "        " << decl.q_params()[i] << " = qubits[" << i << "]\n";
        }
        os_ <<"\n";

        prefix_ = "        ";
        decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });

        // deallocate all ancillas
        for (auto& [name, size] : ancillas_) {
          for (int i = 0; i < size; i++) {
            os_ << prefix_ << eng_ << ".deallocate_qubit(" << name << "[" << i << "])\n";
          }
        }
        ancillas_.clear();
        prefix_ = "";
        os_ << "\n";

        eng_ = tmp;
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
      os_ << "\n";
    }

    void visit(ast::AncillaDecl& decl) {
      os_ << prefix_ << decl.id() << " = " << eng_ << ".allocate_qureg(" << decl.size() << ")\n";
      ancillas_.push_back(std::make_pair(decl.id(), decl.size()));
    }
    
    // Program
    void visit(ast::Program& prog) {
      os_ << "from projectq import MainEngine, ops\n";
      os_ << "from cmath import pi,exp,sin,cos,tan,log as ln,sqrt\n";
      os_ << "import numpy as np\n\n";

      // Convenience functions
      os_ << "def SdagGate(): return ops.Sdag\n";
      os_ << "def TdagGate(): return ops.Tdag\n";
      os_ << "def CNOTGate(): return ops.CNOT\n";
      os_ << "def CZGate(): return ops.CZ\n";
      os_ << "def CCXGate(): return ops.Toffoli\n\n";

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
      os_ << "class ResetGate(ops.FastForwardingGate):\n";
      os_ << "    def __str__(self):\n";
      os_ << "        return \"Reset\"\n\n";
      os_ << "    def __or__(self, qubit):\n";
      os_ << "        ops.Measure | qubit\n";
      os_ << "        if int(qubit):\n";
      os_ << "            ops.X | qubit\n";
      os_ << "Reset = ResetGate()\n\n";

      // Gate declarations
      prog.foreach_stmt([this](auto& stmt) {
          if (typeid(stmt) == typeid(ast::GateDecl))
            stmt.accept(*this);
        });

      if (config_.standalone) { // Standalone simulation
        os_ << "if __name__ == \"__main__\":\n";
        os_ << "    " << eng_ << " = MainEngine()\n";
      } else {                  // Otherwise put file into a function
        os_ << "def " << config_.circuit_name << "(" << eng_ << "):\n";
      }
      prefix_ = "    ";

      // Program body
      prog.foreach_stmt([this](auto& stmt) {
          if (typeid(stmt) != typeid(ast::GateDecl))
            stmt.accept(*this);
        });

      os_ << "\n";
      prefix_ = "";
    }

  private:
    std::ostream& os_;
    config config_;

    std::string prefix_ = "";
    std::string eng_ = "eng";
    std::list<std::pair<std::string, int> > ancillas_{};
    bool ambiguous_ = false;

    // Hack because lambda is reserved by python
    std::string sanitize(const std::string& id) {
      if (id == "lambda") return "lambd";
      else return id;
    }

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
