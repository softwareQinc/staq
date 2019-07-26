#define FMT_HEADER_ONLY = true

#include "mapping/device.hpp"
#include "synthesis/cnot_dihedral.hpp"

#include <vector>
#include <list>
#include <map>
#include <variant>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using namespace synthewareQ::synthesis;
using namespace synthewareQ::mapping;

int main(int argc, char** argv) {

  std::list<phase_term> f {
    { {1, 0, 0}, td::angles::pi_quarter },
    { {0, 1, 0}, td::angles::pi_quarter },
    { {1, 1, 0}, -td::angles::pi_quarter },
    { {0, 0, 1}, td::angles::pi_quarter },
    { {1, 0, 1}, -td::angles::pi_quarter },
    { {0, 1, 1}, -td::angles::pi_quarter },
    { {1, 1, 1}, td::angles::pi_quarter }
  };
      
  linear_op<bool> A {
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

  auto circuit = gray_synth(f, A);

  std::cout << "\n(Unmapped) synthesized circuit:\n";
  for (auto& gate : circuit) {
    std::visit(overloaded {
        [](std::pair<size_t, size_t>& cnot) {
          std::cout << "CNOT " << cnot.first << "," << cnot.second << ";\n";
        },
        [](std::pair<td::angle, size_t>& rz) {
          std::cout << "RZ(" << rz.first << ") " << rz.second << ";\n";
        }}, gate);
  }

  auto mapped_circuit = gray_steiner(f, A, synthewareQ::mapping::square_9q);

  std::cout << "\n(Mapped) synthesized circuit:\n";
  for (auto& gate : mapped_circuit) {
    std::visit(overloaded {
        [](std::pair<size_t, size_t>& cnot) {
          std::cout << "CNOT " << cnot.first << "," << cnot.second << ";\n";
        },
        [](std::pair<td::angle, size_t>& rz) {
          std::cout << "RZ(" << rz.first << ") " << rz.second << ";\n";
        }}, gate);
  }

  return 1;
}
