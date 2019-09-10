#include "parser/position.hpp"
#include "ast/ast.hpp"
#include "ast/semantic.hpp"
#include "parser/parser.hpp"
#include "tools/ast_printer.hpp"
#include "tools/resource_estimator.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <optional>

using namespace synthewareQ;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
    return -1;
  }

  auto program = parser::parse_file(argv[1]);
  if (program) {
    std::cout << "Source:\n" << *program << "\n";

    ast::check_source(*program);
    std::cout << "Passed semantic analysis\n\n";

    std::cout << "AST:\n";
    tools::print_tree(*program);
    std::cout << "\n";

    std::cout << "Resource estimates:\n";
    auto count = tools::estimate_resources(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }
  } else {
    std::cout << "Parsing failed\n";
  }

  return 1;
}
