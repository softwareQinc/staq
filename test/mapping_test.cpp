#include "parser/parser.hpp"

#include "mapping/device.hpp"
#include "mapping/layout/basic.hpp"
#include "mapping/layout/eager.hpp"
#include "mapping/layout/bestfit.hpp"
#include "mapping/mapping/swap.hpp"
#include "mapping/mapping/steiner.hpp"

using namespace synthewareQ;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = parser::parse_file(argv[1]);
  if (program) {
    std::cout << "Unmapped source:\n" << *program << "\n";

    auto physical_layout = mapping::compute_bestfit_layout(mapping::rigetti_8q, *program);
    mapping::apply_layout(physical_layout, *program);
    std::cout << "\nPhysical layout:\n" << *program << "\n";

    mapping::steiner_mapping(mapping::rigetti_8q, *program);
    std::cout << "\nCNOT mapped layout:\n" << *program << "\n";
  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }

  return 1;
}
