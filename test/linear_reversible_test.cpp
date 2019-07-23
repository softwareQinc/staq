#define FMT_HEADER_ONLY = true

#include "mapping/device.hpp"
#include "synthesis/linear_reversible.hpp"

#include <vector>

using namespace synthewareQ::synthesis;

int main(int argc, char** argv) {

  linear_op<bool> mat { { 1, 0, 1, 1, 1, 1, 0, 0, 1 },
                        { 0, 1, 1, 0, 1, 1, 1, 1, 0 },
                        { 1, 0, 0, 0, 1, 1, 1, 0, 1 },
                        { 0, 1, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 1, 1, 1, 1, 0, 1, 1, 1 },
                        { 0, 0, 0, 0, 1, 0, 1, 0, 0 },
                        { 0, 0, 1, 0, 0, 1, 0, 0, 1 },
                        { 1, 1, 1, 1, 0, 0, 1, 1, 0 },
                        { 0, 0, 1, 0, 0, 1, 0, 1, 1 } };

  std::cout << "Parity matrix:\n";
  for (auto i = 0; i < 9; i++) {
    std::cout << "  ";
    for (auto j = 0; j < 9; j++) {
      std::cout << (mat[i][j] ? "1" : "0");
    }
    std::cout << "\n";
  }

  auto res = steiner_gauss(mat, synthewareQ::mapping::square_9q);

  std::cout << "\nCircuit:\n";
  for (auto [i, j] : res) {
    std::cout << "CX " << i << "," << j << "; ";
    mat[j] ^= mat[i];
  }
  std::cout << "\nCNOTs: " << res.size() << "\n";

  std::cout << "\nResulting matrix:\n";
  for (auto i = 0; i < 9; i++) {
    std::cout << "  ";
    for (auto j = 0; j < 9; j++) {
      std::cout << (mat[i][j] ? "1" : "0");
    }
    std::cout << "\n";
  }

  return 1;
}
