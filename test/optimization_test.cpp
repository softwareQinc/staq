#include "parser/parser.hpp"
#include "optimization/rotation_folding.hpp"
#include "tools/resource_estimator.hpp"

#include <unordered_map>

using namespace synthewareQ;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = parser::parse_file(argv[1]);
  if (program) {
    std::cout << "Unoptimized source:\n" << *program << "\n";

    std::cout << "Circuit statistics:\n";
    auto count = tools::estimate_resources(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }
    std::cout << "\n";

    // Do optimization
    optimization::fold_rotations(*program);

    std::cout << "Optimized source:\n" << *program << "\n";

    std::cout << "Circuit statistics:\n";
    count = tools::estimate_resources(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }

  } else {
    std::cout << "Parsing failed\n";
  }

  return 1;
}
