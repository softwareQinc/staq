#include "parser/parser.hpp"
#include "transformations/oracle_synthesizer.hpp"

using namespace synthewareQ;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = parser::parse_file(argv[1]);
  if (program) {
    std::cout << "\nOriginal source:\n" << *program << "\n";

    transformations::synthesize_oracles(*program);
    std::cout << "\nExpanded source:\n" << *program << "\n";
  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }
}
