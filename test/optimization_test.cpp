#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "optimization/rotation_folding.hpp"
#include "qasm/visitors/source_printer.hpp"
#include "qasm/visitors/resource_estimator.hpp"

#include <unordered_map>

using namespace synthewareQ;
using namespace synthewareQ::qasm;

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Input file not specified.\n";
  }

  auto program = synthewareQ::qasm::read_from_file(argv[1]);
  if (program) {
    resource_estimator res;
    auto count_before = res.estimate(*program);

    rotation_folder opt;
    opt.run(*program);

    auto count_after = res.estimate(*program);

    std::cout << "\nBefore optimization:\n";
    for (auto& [name, num] : count_before) {
      std::cout << "  " << name << ": " << num << "\n";
    }

    std::cout << "\nAfter optimization:\n";
    for (auto& [name, num] : count_after) {
      std::cout << "  " << name << ": " << num << "\n";
    }
  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }

  return 1;
}
