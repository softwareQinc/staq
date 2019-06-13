#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "qasm/visitors/ast_printer.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "qasm/visitors/resource_estimator.hpp"

#include <unordered_map>

using namespace synthewareQ::qasm;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = synthewareQ::qasm::read_from_file(argv[1]);
  if (program) {
    std::cout << "Success!\nAST:\n";
    ast_printer printer(std::cout);
    printer.visit(*program);

    std::cout << "\nUntransformed source:\n";
    source_printer src(std::cout);
    src.visit(*program);

    std::cout << "\nResource estimates:\n";
    resource_estimator res;
    auto count = res.estimate(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }
  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }

  return 1;
}
