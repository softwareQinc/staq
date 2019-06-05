#include "parser/parser.hpp"
#include "transformations/desugar.hpp"

using namespace synthewareQ;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = parser::parse_file(argv[1]);
  if (program) {
    std::cout << "\nSugared source:\n" << *program << "\n";

    transformations::desugar(*program);
    std::cout << "\nDesugared source:\n" << *program << "\n";
  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }

  return 1;
}
