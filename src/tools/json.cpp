#include <staq/third_party/CLI/CLI.hpp>
#include "qasmtools/ast/stmt.hpp"
#include "qasmtools/parser/parser.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/* Forward declarations */
// class VarAccess;
// class BExpr;
// class UExpr;
// class PiExpr;
// class IntExpr;
// class RealExpr;
// class VarExpr;
// class MeasureStmt;
// class ResetStmt;
// class IfStmt;
// class UGate;
// class CNOTGate;
// class BarrierGate;
// class DeclaredGate;
// class GateDecl;
// class OracleDecl;
// class RegisterDecl;
// class AncillaDecl;
// class Program;

json jsonify(qasmtools::ast::Stmt& st){
    json j;
    return j;
}


json jsonify(qasmtools::ast::Program& p){
    json j;
    p.foreach_stmt([&](qasmtools::ast::Stmt& st){
        j.push_back(jsonify(st));
    });
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
        //auto jsonified = qasmtools::ast::jsonify(*prog);
        //std::cout << jsonified._json().dump() << std::endl;
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

