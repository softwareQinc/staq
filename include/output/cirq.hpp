/*
 * This file is part of staq.
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

namespace staq {
namespace output {

  /** \brief Equivalent cirq standard gates for qasm standard gates */
  std::unordered_map<std::string, std::string> qasmstd_to_cirq {
    { "id", "cirq.I" },
    { "x", "cirq.X" },
    { "y", "cirq.Y" },
    { "z", "cirq.Z" },
    { "h", "cirq.H" },
    { "s", "cirq.S" },
    { "sdg", "(cirq.S**(-1))" },
    { "t", "cirq.T" },
    { "tdg", "(cirq.T**(-1))" },
    { "cx", "cirq.CNOT" },
    { "cz", "cirq.CZ" },
    { "ccx", "cirq.CCX" },
    { "rx", "cirq.Rx" },
    { "ry", "cirq.Ry" },
    { "rz", "cirq.Rz" },
    { "u1", "cirq.Rz" } };

  class CirqOutputter final : public ast::Visitor {
  public:

    struct config {
      std::string circuit_name = "circuit";
    };

    CirqOutputter(std::ostream& os) : Visitor(), os_(os) {}
    CirqOutputter(std::ostream& os, const config& params) : Visitor(), os_(os), config_(params) {}
    ~CirqOutputter() = default;

    void run(ast::Program& prog) {
      prefix_ = "";
      ambiguous_ = false;

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
      os_ << prefix_ << "cirq.measure(" << stmt.q_arg() << ", key=\"" << stmt.c_arg() << "\")"; 
      os_ << ",\n";
    }

    void visit(ast::ResetStmt& stmt) {
      throw std::logic_error("Qubit reset not yet supported in cirq");
    }

    void visit(ast::IfStmt& stmt) {
      throw std::logic_error("Classical control not yet supported in cirq");
    }

    // Gates
    void visit(ast::UGate& gate) {
      os_ << prefix_ << "UGate(";
      gate.theta().accept(*this);
      os_ << ", ";
      gate.phi().accept(*this);
      os_ << ", ";
      gate.lambda().accept(*this);
      os_ << ")(";
      gate.arg().accept(*this);
      os_ << "),\n";
    }

    void visit(ast::CNOTGate& gate) {
      os_ << prefix_ << "cirq.CNOT(";
      gate.ctrl().accept(*this);
      os_ << ", ";
      gate.tgt().accept(*this);
      os_ << "),\n";
    }

    void visit(ast::BarrierGate&) { }

    void visit(ast::DeclaredGate& gate) {
      os_ << prefix_;

      if (auto it = qasmstd_to_cirq.find(gate.name()); it != qasmstd_to_cirq.end())
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

      os_ << "(";
      for (int i = 0; i < gate.num_qargs(); i++) {
        if (i > 0)
          os_ << ", ";
        gate.qarg(i).accept(*this);
      }
      os_ << "),\n";
    }

    // Declarations
    void visit(ast::GateDecl& decl) {
      if (decl.is_opaque())
        throw std::logic_error("Opaque declarations not supported");
      
      if (qasmstd_to_cirq.find(decl.id()) == qasmstd_to_cirq.end()) {

        if (decl.c_params().size() == 0)
          os_ << "class " << decl.id() << "Init(cirq.Gate):\n";
        else
          os_ << "class " << decl.id() << "(cirq.Gate):\n";

        // Class instantiation
        os_ << "    def __init__(self";
        for (auto i = 0; i < decl.c_params().size(); i++) {
          os_ << ", " << sanitize(decl.c_params()[i]);
        }
        os_ << "):\n";
        for (auto param : decl.c_params()) {
          auto tmp = sanitize(param);
          os_ << "        self." << tmp << " = " << tmp << "\n";
        }
        os_ << "        return\n";
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

        // Implementation as an override of ||
        os_ << "    def _decompose_(self, qubits):\n";
        os_ << "        if len(qubits) != " << decl.q_params().size() << ":\n";
        os_ << "            raise TypeError(\"Expected " << decl.q_params().size()
                                                         << " qubits, given \" + len(qubits))\n";
        for (auto i = 0; i < decl.q_params().size(); i++) {
          os_ << "        " << decl.q_params()[i] << " = qubits[" << i << "]\n";
        }
        os_ <<"\n";

        os_ << "        return [\n";
        prefix_ = "            ";
        decl.foreach_stmt([this](auto& stmt) { stmt.accept(*this); });
        os_ << "        ]\n";

        // num_qubits (abstract method) override
        os_ << "    def num_qubits(self):\n";
        os_ << "        return " << decl.q_params().size();
        os_ << "\n";

        if (decl.c_params().size() == 0)
          os_ << decl.id() << " = " << decl.id() << "Init()\n";

        prefix_ = "";
        os_ << "\n";

      }
    }

    void visit(ast::OracleDecl& decl) {
      throw std::logic_error("Cirq has no support for oracle declarations via logic files");
    }

    void visit(ast::RegisterDecl& decl) {
      if (decl.is_quantum()) {
        os_ << prefix_ << decl.id() << " = [cirq.NamedQubit(\"" << decl.id() << "[i]\") for i in range(";
	os_ << decl.size() << ")]\n";
      }
    }

    void visit(ast::AncillaDecl& decl) {
      throw std::logic_error("Cirq has no support for local ancillas");
    }
    
    // Program
    void visit(ast::Program& prog) {
      os_ << "import cirq\n";
      os_ << "import numpy as np\n";
      os_ << "from cmath import pi,exp,sin,cos,tan,log as ln,sqrt\n\n";

      // QASM U gate
      os_ << "class UGate(cirq.SingleQubitMatrixGate):\n";
      os_ << "    def __init__(self, theta, phi, lambd):\n";
      os_ << "        mat = np.matrix([[exp(-1j*(phi+lambd)/2)*cos(theta/2),\n";
      os_ << "                          -exp(-1j*(phi-lambd)/2)*sin(theta/2)],\n";
      os_ << "                         [exp(1j*(phi-lambd)/2)*sin(theta/2),\n";
      os_ << "                          exp(1j*(phi+lambd)/2)*cos(theta/2)]])\n";
      os_ << "        cirq.SingleQubitMatrixGate.__init__(self, mat)\n";
      os_ << "        self.theta = theta\n";
      os_ << "        self.phi = phi\n";
      os_ << "        self.lambd = lambd\n\n";
      os_ << "    def __str__(self):\n";
      os_ << "        return str(self.__class__.__name__) + \"(\" + str(self.theta) + \",\" \\\n";
      os_ << "               + str(self.phi) + \",\" + str(self.lambd) + \")\"\n\n";
      os_ << "    def __eq__(self, other):\n";
      os_ << "        if isinstance(other, self.__class__):\n";
      os_ << "            return self.theta == other.theta \\\n";
      os_ << "                   & self.phi == other.phi \\\n";
      os_ << "                   & self.lambd == other.lambd\n";
      os_ << "        else:\n";
      os_ << "            return False\n\n";
      os_ << "    def __ne__(self, other):\n";
      os_ << "        return not self.__eq__(other)\n\n";

      // Gate & qubit declarations
      prog.foreach_stmt([this](auto& stmt) {
          if (typeid(stmt) == typeid(ast::GateDecl) || typeid(stmt) == typeid(ast::RegisterDecl))
            stmt.accept(*this);
        });

      os_ << config_.circuit_name << " = cirq.Circuit()\n";
      os_ << config_.circuit_name << ".append([\n";
      prefix_ = "    ";

      // Program body
      prog.foreach_stmt([this](auto& stmt) {
          if (typeid(stmt) != typeid(ast::GateDecl) && typeid(stmt) != typeid(ast::RegisterDecl))
            stmt.accept(*this);
        });

      os_ << "])\n\nprint(" << config_.circuit_name << ")";
      prefix_ = "";
    }

  private:
    std::ostream& os_;
    config config_;

    std::string prefix_ = "";
    bool ambiguous_ = false;

    // Hack because lambda is reserved by python
    std::string sanitize(const std::string& id) {
      if (id == "lambda") return "lambd";
      else return id;
    }

  };

  void output_cirq(ast::Program& prog) {
    CirqOutputter outputter(std::cout);
    outputter.run(prog);
  }

  void write_cirq(ast::Program& prog, std::string fname) {
    std::ofstream ofs;
    ofs.open(fname);

    if (!ofs.good()) {
      std::cerr << "Error: failed to open output file " << fname << "\n";
    } else {
      CirqOutputter outputter(ofs);
      outputter.run(prog);
    }

    ofs.close();
  }

}
}
