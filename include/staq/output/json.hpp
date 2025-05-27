/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2025 softwareQ Inc. All rights reserved.
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

#ifndef OUTPUT_JSON_HPP_
#define OUTPUT_JSON_HPP_

#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <staq/third_party/CLI/CLI.hpp>
#include "qasmtools/ast/decl.hpp"
#include "qasmtools/ast/stmt.hpp"
#include "qasmtools/ast/var.hpp"
#include "qasmtools/parser/parser.hpp"

#include <typeinfo>

#include "qasmtools/ast/ast.hpp"

namespace staq {
namespace output {

using namespace qasmtools::ast;
using json = nlohmann::json;

/**
 * \class staq::output::JSONOutputter
 * \brief Visitor for converting a QASM AST to JSON
 */
class JSONOutputter final : public Visitor {
  public:
    JSONOutputter() = default;
    ~JSONOutputter() = default;

    void run(Program& prog) { prog.accept(*this); }

    void visit(UGate& gd) override {
        json j;
        j["type"] = "Gate";
        JSONOutputter arg0, arg1, arg2, arg3;
        gd.arg().accept(arg0);
        gd.arg().accept(arg1);
        gd.arg().accept(arg2);
        gd.arg().accept(arg3);
        j["name"] = "UGate";
        j["qargs"] = {arg0.json_val()};
        j["cargs"] = {arg1.json_val(), arg2.json_val(), arg3.json_val()};
        json_.push_back(j);
    }
    void visit(CNOTGate& gd) override {
        json j;
        j["type"] = "Gate";
        JSONOutputter arg0, arg1;
        gd.ctrl().accept(arg0);
        gd.tgt().accept(arg1);
        j["name"] = "CNOTGate";
        j["qargs"] = {arg0.json_val(), arg1.json_val()};
    }
    void visit(BarrierGate& gd) override {
        using namespace qasmtools::ast;
        json j;
        j["type"] = "Gate";
        j["name"] = "BarrierGate";
        j["qargs"] = {};
        gd.foreach_arg([&j](VarAccess& va) {
            JSONOutputter jva;
            va.accept(jva);
            j["qargs"].push_back(jva.json_val());
        });
    }
    void visit(DeclaredGate& gd) override {
        using namespace qasmtools::ast;
        json j;
        j["type"] = "Gate";
        j["name"] = gd.name();
        j["qargs"] = {};
        j["cargs"] = {};
        gd.foreach_qarg([&j](VarAccess& va) {
            JSONOutputter jva;
            va.accept(jva);
            j["qargs"].push_back(jva.json_val());
        });
        gd.foreach_carg([&j](Expr& va) {
            JSONOutputter jva;
            va.accept(jva);
            j["cargs"].push_back(jva.json_val());
        });
    }

    void visit(AncillaDecl& ad) override {
        json j;
        j["type"] = "AncillaDecl";
        j["name"] = ad.id();
        j["size"] = ad.size();
        j["is_dirty"] = (ad.is_dirty() ? 1 : 0);
        json_.push_back(j);
    }

    void visit(RegisterDecl& rd) override {
        json j;
        j["type"] = "RegisterDecl";
        j["name"] = rd.id();
        j["is_quantum"] = (rd.is_quantum() ? 1 : 0);
        j["size"] = rd.size();
        json_.push_back(j);
    }

    void visit(OracleDecl& od) override {
        // TODO: verify that this is doing what it needs to do
        json j;
        j["type"] = "OracleDecl";
        j["name"] = od.fname();
        j["params"] = od.params();
        json_.push_back(j);
    }

    void visit(IfStmt& ist) {
        // TODO: Improve this later.
        json j;
        std::stringstream in;
        ist.pretty_print(in, false);
        j["type"] = "IfStmt";
        j["name"] = "If";
        j["body"] = in.str();
        json_.push_back(j);
    }

    void visit(ResetStmt& rst) override {
        json j;
        JSONOutputter arg_json_outputter;
        rst.arg().accept(arg_json_outputter);
        j["type"] = "ResetStmt";
        j["name"] = "Reset";
        j["qarg"] = arg_json_outputter.json_val();
        json_.push_back(j);
    }

    void visit(MeasureStmt& mst) override {
        json j;
        JSONOutputter arg_json_1, arg_json_2;
        mst.q_arg().accept(arg_json_1);
        mst.c_arg().accept(arg_json_2);
        j["type"] = "MeasureStmt";
        j["name"] = "Measurement";
        j["qarg"] = arg_json_1.json_val();
        j["carg"] = arg_json_2.json_val();
        json_.push_back(j);
    }

    void visit(VarAccess& va) override {
        json j;
        j["type"] = "VarAccess";
        j["name"] = "qubit";
        j["symbol"] = va.var();
        j["offset"] = {};
        if (va.offset().has_value()) {
            j["offset"].push_back(va.offset().value());
        }
        json_.push_back(j);
    }

    // Expressions
    void visit(BExpr& e) override {};
    void visit(UExpr& e) override {};
    void visit(PiExpr& e) override {};
    void visit(IntExpr& e) override {};
    void visit(RealExpr& e) override {};
    void visit(VarExpr& e) override {};

    void visit(Expr& expr) {
        json j;
        std::stringstream in;
        expr.pretty_print(in);
        auto ev = expr.constant_eval();
        j["type"] = "Expr";
        j["expr"] = in.str();
        j["val"] = {};
        if (ev.has_value()) {
            j["val"].push_back(ev.value());
        }
        json_.push_back(j);
    }

    // void visit(qasmtools::ast::Gate& g) override {
    //     using namespace qasmtools::ast;
    //     json j;
    //
    //     j["type"] = "Gate";
    //     if (UGate* gd = dynamic_cast<UGate*>(&g)) {
    //         JSONOutputter arg0, arg1, arg2, arg3;
    //         gd->arg().accept(arg0);
    //         gd->arg().accept(arg1);
    //         gd->arg().accept(arg2);
    //         gd->arg().accept(arg3);
    //         j["name"] = "UGate";
    //         j["qargs"] = {arg0.json_val()};
    //         j["cargs"] = {arg1.json_val(), arg2.json_val(), arg3.json_val()};
    //     } else if (CNOTGate* gd = dynamic_cast<CNOTGate*>(&g)) {
    //         //(typeid(g) == typeid(CNOTGate)) {
    //         JSONOutputter arg0, arg1;
    //         gd->ctrl().accept(arg0);
    //         gd->tgt().accept(arg1);
    //         j["name"] = "CNOTGate";
    //         j["qargs"] = {arg0.json_val(), arg1.json_val()};
    //     } else if (BarrierGate* gd = dynamic_cast<BarrierGate*>(&g)) {
    //         j["name"] = "BarrierGate";
    //         j["qargs"] = {};
    //         gd->foreach_arg([&j](VarAccess& va) {
    //             JSONOutputter jva;
    //             va.accept(jva);
    //             j["qargs"].push_back(jva.json_val());
    //         });
    //     } else if (DeclaredGate* gd = dynamic_cast<DeclaredGate*>(&g)) {
    //         j["name"] = gd->name();
    //         j["qargs"] = {};
    //         j["cargs"] = {};
    //         gd->foreach_qarg([&j](VarAccess& va) {
    //             JSONOutputter jva;
    //             va.accept(jva);
    //             j["qargs"].push_back(jva.json_val());
    //         });
    //         gd->foreach_carg([&j](Expr& va) {
    //             JSONOutputter jva;
    //             va.accept(jva);
    //             j["cargs"].push_back(jva.json_val());
    //         });
    //     } else {
    //         throw "";
    //     }
    //     json_.push_back(j);
    // }

    void visit(GateDecl& gd) override {
        using namespace qasmtools::ast;
        json j;
        j["type"] = "GateDecl";
        j["name"] = gd.id();
        j["q_params"] = gd.q_params();
        j["c_params"] = gd.c_params();
        j["body"] = {}; // process the body of a GateDecl;
        gd.foreach_stmt([&j](Stmt& st) {
            JSONOutputter jst;
            st.accept(jst);
            j["body"].push_back(jst.json_val());
        });
        json_.push_back(j);
    }

    void visit(Program& p) override {
        using namespace qasmtools::ast;
        p.foreach_stmt([&](Stmt& st) {
            JSONOutputter jst;
            st.accept(jst);
            json_.push_back(jst.json_val());
        });
    }

    json json_val() { return json_; }

  private:
    json json_;
};

} /* namespace output */
} /* namespace staq */

#endif /* OUTPUT_JSON_HPP_ */
