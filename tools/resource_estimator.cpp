/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#define FMT_HEADER_ONLY = true

#include "qasm/qasm.hpp"
#include "qasm/visitors/resource_estimator.hpp"

using namespace synthewareQ;

int main() {

  auto program = qasm::read_from_stdin();
  if (program) {
    qasm::resource_estimator res;

    std::cout << "Resources used:\n";
    auto count = res.estimate(*program);
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }
  } else {
    std::cerr << "Parsing failed\n";
  }

  return 1;
}
