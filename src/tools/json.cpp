#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <staq/third_party/CLI/CLI.hpp>
#include "qasmtools/ast/decl.hpp"
#include "qasmtools/ast/stmt.hpp"
#include "qasmtools/ast/var.hpp"
#include "qasmtools/parser/parser.hpp"

using json = nlohmann::json;

/* Forward declarations */
// [x] class VarAccess;
// [x] class BExpr,UExpr,PiExpr,IntExpr,RealExpr,VarExpr;
// [ ] class MeasureStmt,ResetStmt,IfStmt,
// [x] class UGate,CNOTGate,BarrierGate,DeclaredGate,
// [ ] class GateDecl,OracleDecl,RegisterDecl,AncillaDecl;
// [ ] class Program;

json jsonify(qasmtools::ast::VarAccess&);
json jsonify(qasmtools::ast::Expr&);
json jsonify(qasmtools::ast::Gate&);
json jsonify(qasmtools::ast::GateDecl&);
json jsonify(qasmtools::ast::Stmt&);
json jsonify(qasmtools::ast::MeasureStmt&);
json jsonify(qasmtools::ast::ResetStmt&);
json jsonify(qasmtools::ast::OracleDecl&);
json jsonify(qasmtools::ast::RegisterDecl&);
json jsonify(qasmtools::ast::AncillaDecl&);
json jsonify(qasmtools::ast::IfStmt&);
json jsonify(qasmtools::ast::Program&);

json jsonify(qasmtools::ast::AncillaDecl& ad) {
    json j;
    j["type"] = "AncillaDecl";
    j["name"] = ad.id();
    j["size"] = ad.size();
    j["is_dirty"] = (ad.is_dirty() ? 1 : 0);
    return j;
}

json jsonify(qasmtools::ast::RegisterDecl& rd) {
    json j;
    j["type"] = "RegisterDecl";
    j["name"] = rd.id();
    j["is_quantum"] = (rd.is_quantum() ? 1 : 0);
    j["size"] = rd.size();
    return j;
}

json jsonify(qasmtools::ast::OracleDecl& od) {
    // TODO: verify that this is doing what it needs to do
    json j;
    j["type"] = "OracleDecl";
    j["name"] = od.fname();
    j["params"] = od.params();
    return j;
}

json jsonify(qasmtools::ast::IfStmt& ist) {
    // TODO: Improve this later.
    json j;
    std::stringstream in;
    ist.pretty_print(in, false);
    j["type"] = "IfStmt";
    j["name"] = "If";
    j["body"] = in.str();
    return j;
}

json jsonify(qasmtools::ast::ResetStmt& rst) {
    json j;
    j["type"] = "ResetStmt";
    j["name"] = "Reset";
    j["qarg"] = jsonify(rst.arg());
    return j;
}

json jsonify(qasmtools::ast::MeasureStmt& mst) {
    json j;
    j["type"] = "MeasureStmt";
    j["name"] = "Measurement";
    j["qarg"] = jsonify(mst.q_arg());
    j["carg"] = jsonify(mst.c_arg());
    return j;
}

json jsonify(qasmtools::ast::VarAccess& va) {
    json j;
    j["type"] = "VarAccess";
    j["name"] = "qubit";
    j["symbol"] = va.var();
    j["offset"] = {};
    if (va.offset().has_value()) {
        j["offset"].push_back(va.offset().value());
    }
    return j;
}

json jsonify(qasmtools::ast::Expr& expr) {
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
    return j;
}

json jsonify(qasmtools::ast::Gate& g) {
    using namespace qasmtools::ast;
    json j;
    j["type"] = "Gate";
    if (UGate* gd = dynamic_cast<UGate*>(&g)) {
        j["name"] = "UGate";
        j["qargs"] = {jsonify(gd->arg())};
        j["cargs"] = {jsonify(gd->theta()), jsonify(gd->phi()),
                      jsonify(gd->lambda())};
    } else if (CNOTGate* gd = dynamic_cast<CNOTGate*>(&g)) {
        j["name"] = "CNOTGate";
        j["qargs"] = {jsonify(gd->ctrl()), jsonify(gd->tgt())};
    } else if (BarrierGate* gd = dynamic_cast<BarrierGate*>(&g)) {
        j["name"] = "BarrierGate";
        j["qargs"] = {};
        gd->foreach_arg(
            [&j](VarAccess& va) { j["qargs"].push_back(jsonify(va)); });
    } else if (DeclaredGate* gd = dynamic_cast<DeclaredGate*>(&g)) {
        j["name"] = gd->name();
        j["qargs"] = {};
        j["cargs"] = {};
        gd->foreach_qarg(
            [&j](VarAccess& va) { j["qargs"].push_back(jsonify(va)); });
        gd->foreach_carg([&j](Expr& va) { j["cargs"].push_back(jsonify(va)); });
    } else {
        throw "";
    }
    return j;
}

json jsonify(qasmtools::ast::GateDecl& gd) {
    using namespace qasmtools::ast;
    json j;
    j["type"] = "GateDecl";
    j["name"] = gd.id();
    j["q_params"] = gd.q_params();
    j["c_params"] = gd.c_params();
    j["body"] = {}; // process the body of a GateDecl;
    gd.foreach_stmt([&j](Stmt& st) { j["body"].push_back(jsonify(st)); });
    return j;
}

json jsonify(qasmtools::ast::Stmt& st) {
    using namespace qasmtools::ast;
    if (GateDecl* gd = dynamic_cast<GateDecl*>(&st)) {
        return jsonify(*gd);
    } else if (Gate* gd = dynamic_cast<Gate*>(&st)) {
        return jsonify(*gd);
    } else if (Gate* gd = dynamic_cast<Gate*>(&st)) {
        return jsonify(*gd);
    } else if (MeasureStmt* gd = dynamic_cast<MeasureStmt*>(&st)) {
        return jsonify(*gd);
    } else if (ResetStmt* gd = dynamic_cast<ResetStmt*>(&st)) {
        return jsonify(*gd);
    } else if (OracleDecl* gd = dynamic_cast<OracleDecl*>(&st)) {
        return jsonify(*gd);
    } else if (RegisterDecl* gd = dynamic_cast<RegisterDecl*>(&st)) {
        return jsonify(*gd);
    } else if (AncillaDecl* gd = dynamic_cast<AncillaDecl*>(&st)) {
        return jsonify(*gd);
    } else if (IfStmt* gd = dynamic_cast<IfStmt*>(&st)) {
        return jsonify(*gd);
    } else {
        throw "";
    }
}

json jsonify(qasmtools::ast::Program& p) {
    using namespace qasmtools::ast;
    json j;
    p.foreach_stmt([&](Stmt& st) { j.push_back(jsonify(st)); });
    return j;
}

int main(int argc, char** argv) {
    // using namespace staq;
    using qasmtools::parser::parse_file;

    if (argc == 1) {
        std::cout << "Usage: staq [PASSES/OPTIONS] FILE.qasm\n"
                  << "Run with --help for more information.\n";
        return 0;
    }
    std::string input_qasm;

    CLI::App app{"staq -- A full-stack quantum processing toolkit"};
    app.allow_extras();

    app.add_option("FILE.qasm", input_qasm, "OpenQASM circuit")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);
    try {
        auto prog = parse_file(input_qasm); // Default std_include=true
        if (!prog) {
            // This case might not be reached if parse_file throws on error,
            // but good to keep for parsers that might return nullptr.
            std::cerr << "Error: failed to parse \"" << input_qasm
                      << "\" (parser returned null).\n";
            return 1;
        }

        // create json
        // auto jsonified = qasmtools::ast::jsonify(*prog);
        // std::cout << jsonified._json().dump() << std::endl;
        json jsonified = jsonify(*prog);
        std::cout << jsonified.dump() << std::endl;
        return 0;
    } catch (const qasmtools::parser::ParseError& e) {
        std::cerr << "ParseError: " << e.what() << std::endl;
        std::cerr
            << "Parsing failed. The error messages above this one (if any) "
               "from the parser provide details about the location of the "
               "syntax error(s) in the QASM file."
            << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
