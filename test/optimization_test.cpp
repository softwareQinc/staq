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
    source_printer printer(std::cout);

    std::cout << "Unoptimized source:\n";
    printer.visit(*program);

    std::cout << "\nCircuit statistics:\n";
    auto count = res.estimate(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }

    // Do optimization
    rotation_fold(*program);

    std::cout << "\n\nOptimized source:\n";
    printer.visit(*program);

    std::cout << "\nCircuit statistics:\n";
    count = res.estimate(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }

  } else {
    std::cout << "Parsing of file \"" << argv[1] << "\" failed\n";
  }

  return 1;
}
