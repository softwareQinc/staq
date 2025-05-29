#include "staq/output/json.hpp"
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include "qasmtools/ast/decl.hpp"
#include "qasmtools/ast/stmt.hpp"
#include "qasmtools/ast/var.hpp"
#include "qasmtools/parser/parser.hpp"

int main(int argc, char** argv) {
    using qasmtools::parser::parse_stdin;
    std::string filename;

    CLI::App app{"QASM to JSON converter"};
    app.add_option("-o,--output", filename, "Output to a file");

    CLI11_PARSE(app, argc, argv);
    auto prog = parse_stdin();
    if (prog) {
        staq::output::JSONOutputter jo;
        prog->accept(jo);
        if (filename.empty()) {
            std::cout << jo.json_val().dump() << std::endl;
        } else {
            std::fstream fout(filename);
            fout << jo.json_val().dump() << std::endl;
            fout.close();
        }
    } else {
        std::cerr << "Error: failed to parse " << std::endl;
        return 1;
    }
    return 0;
}
