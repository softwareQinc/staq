#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "transformations/desugarer.hpp"

#include <unordered_map>

using namespace synthewareQ;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = qasm::read_from_file(argv[1]);
  if (program) {
    std::cout << "\nSugared source:\n";
    qasm::source_printer src(std::cout);
    src.visit(*program);

    std::cout << "\nDesugared source:\n";
    transformations::desugar(program.get());
    src.visit(*program);
  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }

  return 1;
}
