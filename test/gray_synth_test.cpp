#include "mapping/device.hpp"
#include "synthesis/cnot_dihedral.hpp"
#include "utils/templates.hpp"
#include "utils/angle.hpp"

#include <vector>
#include <list>
#include <map>
#include <variant>

using namespace synthewareQ;

int main(int argc, char** argv) {

  std::list<synthesis::phase_term> f {
    { {1, 0, 0}, utils::angles::pi_quarter },
    { {0, 1, 0}, utils::angles::pi_quarter },
    { {1, 1, 0}, -utils::angles::pi_quarter },
    { {0, 0, 1}, utils::angles::pi_quarter },
    { {1, 0, 1}, -utils::angles::pi_quarter },
    { {0, 1, 1}, -utils::angles::pi_quarter },
    { {1, 1, 1}, utils::angles::pi_quarter }
  };
      
  synthesis::linear_op<bool> A {
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
  };

  std::cout << "Phase terms:\n";
  for (auto& [vec, angle] : f) {
    std::cout << "  " << angle << "*(";
    for (auto i = 0; i < A.size(); i++) {
      if (vec[i]) {
        if (i != 0) std::cout << "+";
        std::cout << "x" << i;
      }
    }
    std::cout << ")\n";
  }
  std::cout << "\nLinear permutation:\n";
  for (auto i = 0; i < A.size(); i++) {
    std::cout << "  ";
    for (auto j = 0; j < A[i].size(); j++) {
      std::cout << (A[i][j] ? "1" : "0");
    }
    std::cout << "\n";
  }

  auto circuit = synthesis::gray_synth(f, A);

  std::cout << "\n(Unmapped) synthesized circuit:\n";
  for (auto& gate : circuit) {
    std::visit(utils::overloaded {
        [](std::pair<int, int>& cnot) {
          std::cout << "CNOT " << cnot.first << "," << cnot.second << ";\n";
        },
        [](std::pair<utils::Angle, int>& rz) {
          std::cout << "RZ(" << rz.first << ") " << rz.second << ";\n";
        }}, gate);
  }

  auto mapped_circuit = synthesis::gray_steiner(f, A, synthewareQ::mapping::square_9q);

  std::cout << "\n(Mapped) synthesized circuit:\n";
  for (auto& gate : mapped_circuit) {
    std::visit(utils::overloaded {
        [](std::pair<int, int>& cnot) {
          std::cout << "CNOT " << cnot.first << "," << cnot.second << ";\n";
        },
        [](std::pair<utils::Angle, int>& rz) {
          std::cout << "RZ(" << rz.first << ") " << rz.second << ";\n";
        }}, gate);
  }

  return 1;
}
