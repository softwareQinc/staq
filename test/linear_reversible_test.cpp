#include "mapping/device.hpp"
#include "synthesis/linear_reversible.hpp"

#include <vector>
#include <list>
#include <map>

using namespace synthewareQ::synthesis;
using namespace synthewareQ::mapping;

std::list<std::pair<int, int> > map(const std::list<std::pair<int, int> >& circuit, Device& d) {
  std::list<std::pair<int, int> > ret;
  
  for (auto [ctrl, tgt] : circuit) {
    path cnot_chain = d.shortest_path(ctrl, tgt);
    auto i = ctrl;

    for (auto j : cnot_chain) {
      if (j == tgt) {
        ret.push_back(std::make_pair(i, j));
        break;
      } else if (j != i) {
        ret.push_back(std::make_pair(i, j));
        ret.push_back(std::make_pair(j, i));
        ret.push_back(std::make_pair(i, j));
      }
      i = j;
    }

    auto it = std::next(cnot_chain.rbegin());
    i = *it;
    ++it;
    for (; it != cnot_chain.rend(); it++) {
      ret.push_back(std::make_pair(i, *it));
      ret.push_back(std::make_pair(*it, i));
      ret.push_back(std::make_pair(i, *it));

      i = *it;
    }

  }

  return ret;
}


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

  auto res_jordan = gauss_jordan(mat);
  auto res_jordan_mapped = map(res_jordan, synthewareQ::mapping::square_9q);
  auto res_gauss = gaussian_elim(mat);
  auto res_gauss_mapped = map(res_gauss, synthewareQ::mapping::square_9q);
  auto res_steiner = steiner_gauss(mat, synthewareQ::mapping::square_9q);

  std::cout << "\nUnmapped (Gauss-Jordan) circuit:\n";
  for (auto [i, j] : res_jordan) {
    std::cout << "CX " << i << "," << j << "; ";
  }
  std::cout << "\nCNOTs: " << res_jordan.size() << "\n";

  std::cout << "\nMapped (Gauss-Jordan) circuit:\n";
  for (auto [i, j] : res_jordan_mapped) {
    std::cout << "CX " << i << "," << j << "; ";
  }
  std::cout << "\nCNOTs: " << res_jordan_mapped.size() << "\n";

  std::cout << "\nUnmapped (Gaussian elimination) circuit:\n";
  for (auto [i, j] : res_gauss) {
    std::cout << "CX " << i << "," << j << "; ";
  }
  std::cout << "\nCNOTs: " << res_gauss.size() << "\n";

  std::cout << "\nMapped (Gaussian elimination) circuit:\n";
  for (auto [i, j] : res_gauss_mapped) {
    std::cout << "CX " << i << "," << j << "; ";
  }
  std::cout << "\nCNOTs: " << res_gauss_mapped.size() << "\n";

  std::cout << "\nMapped (Steiner-Gauss) circuit:\n";
  for (auto [i, j] : res_steiner) {
    std::cout << "CX " << i << "," << j << "; ";
    mat[j] ^= mat[i];
  }
  std::cout << "\nCNOTs: " << res_steiner.size() << "\n";

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
