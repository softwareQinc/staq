/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

#include "mapping/device.hpp"

#include <vector>
#include <list>

#define debug false

namespace synthewareQ {
namespace synthesis {

  template<typename T>
  using linear_op = std::vector<std::vector<T> >;

  // TODO: replace vector<bool> with a custom packed bitvector that supports faster
  // bitwise operations
  std::vector<bool>& operator^=(std::vector<bool>& A, const std::vector<bool>& B) {
    for (auto i = 0; i < A.size(); i++) {
      A[i] = B[i] ^ A[i];
    }
    return A;
  }

  std::list<std::pair<size_t, size_t> > steiner_gauss(linear_op<bool> mat, mapping::device& d) {
    std::list<std::pair<size_t, size_t> > ret;

    // Stores the location of the leading 1 for each row
    std::vector<size_t> leading_one(mat.size(), -1);

    // Stores whether a parent with an earlier leading
    // zero has been added to the row
    std::vector<bool> leading_parent(mat.size(), false);

    for (auto i = 0; i < mat[0].size(); i++) {
      std::fill(leading_parent.begin(), leading_parent.end(), false);

      // Debug
      if (debug) {
        std::cout << "Column " << i << ":\n";
        std::cout << "  Matrix:\n";
        for (auto i = 0; i < 9; i++) {
          std::cout << "    ";
          for (auto j = 0; j < 9; j++) {
            std::cout << (mat[i][j] ? "1" : "0");
          }
          std::cout << "\n";
        }
        std::cout << "\n";
      }

      // Phase 0: Find pivot & swap
      size_t pivot = -1;
      size_t dist;
      std::list<std::pair<size_t, size_t> > swap;
      for (auto j = i; j < mat.size(); j++) {
        if (mat[j][i] == true) {
          if (pivot == -1 || d.distance(j, i) < dist) {
            pivot = j;
            dist = d.distance(j, i);
          }
        }
      }
      if (pivot == -1) {
        std::cerr << "Error: linear operator is not invertible\n";
        return ret;
      }

      mat[pivot].swap(mat[i]);
      // Swap operations
      for (auto j : d.shortest_path(pivot, i)) {
        if (j != pivot) {
          swap.push_back(std::make_pair(pivot, j));
          swap.push_back(std::make_pair(j, pivot));
          swap.push_back(std::make_pair(pivot, j));
          pivot = j;
        }
      }
      // So that the other rows do not end up out of order
      if (!swap.empty()) {
        swap.insert(swap.end(), std::next(swap.rbegin(), 3), swap.rend());
      }
      if (debug) {
        std::cout << "  Swapping:\n    ";
        for (auto [ctrl, tgt] : swap) std::cout << "CNOT " << ctrl << "," << tgt << "; ";
        std::cout << "\n";
        std::cout << "  Matrix:\n";
        for (auto i = 0; i < 9; i++) {
          std::cout << "    ";
          for (auto j = 0; j < 9; j++) {
            std::cout << (mat[i][j] ? "1" : "0");
          }
          std::cout << "\n";
        }
      }

      // Phase 1: Compute steiner tree covering the 1's in column i
      std::list<size_t> pivots;
      for (auto j = 0; j < mat.size(); j++) {
        if (j != i && mat[j][i] == true) pivots.push_back(j);
      }
      auto s_tree = d.steiner(pivots, pivot);

      if (debug) {
        std::cout << "  Pivot: " << pivot << "\n";
        std::cout << "  Steiner tree:\n    ";
        for (auto [i, j] : s_tree) std::cout << "(" << i << "," << j << "), ";
        std::cout << "\n";
      }

      std::list<std::pair<size_t, size_t> > compute;
      // Phase 2: Propagate 1's to column i for each Steiner point
      for (auto& [ctrl, tgt] : s_tree) {
        if (mat[tgt][i] == false) {
          mat[tgt] ^= mat[ctrl];
          compute.push_back(std::make_pair(ctrl, tgt));

          auto leading_edge = leading_one[tgt] == -1 ? i : leading_one[tgt];
          if (leading_parent[ctrl] || (leading_one[ctrl] != -1 && leading_one[ctrl] != leading_edge)) {
            leading_parent[tgt] = true;
          }
        }
      }

      if (debug) {
        std::cout << "  Filling with 1's:\n    ";
        for (auto [ctrl, tgt] : compute) std::cout << "CNOT " << ctrl << "," << tgt << "; ";
        std::cout << "\n";
        std::cout << "  Matrix:\n";
        for (auto i = 0; i < 9; i++) {
          std::cout << "    ";
          for (auto j = 0; j < 9; j++) {
            std::cout << (mat[i][j] ? "1" : "0");
          }
          std::cout << "\n";
        }
      }

      // Phase 3: Empty all 1's from column i in the Steiner tree
      for (auto it = s_tree.rbegin(); it != s_tree.rend(); it++) {
        auto ctrl = it->first;
        auto tgt = it->second;

        mat[tgt] ^= mat[ctrl];
        compute.push_back(std::make_pair(ctrl, tgt));

        auto leading_edge = leading_one[tgt] == -1 ? i : leading_one[tgt];
        if (leading_parent[ctrl] || (leading_one[ctrl] != -1 && leading_one[ctrl] != leading_edge)) {
          leading_parent[tgt] = true;
        }
      }

      if (debug) {
        std::cout << "  Full compute cycle:\n    ";
        for (auto [ctrl, tgt] : compute) std::cout << "CNOT " << ctrl << "," << tgt << "; ";
        std::cout << "\n";
        std::cout << "  Matrix:\n";
        for (auto i = 0; i < 9; i++) {
          std::cout << "    ";
          for (auto j = 0; j < 9; j++) {
            std::cout << (mat[i][j] ? "1" : "0");
          }
          std::cout << "\n";
        }
      }

      // Phase 4: For each node that has had a parent with an earlier leading
      // zero added to it, repeat the previous steps to undo the additions
      std::list<std::pair<size_t, size_t> > uncompute;
      for (auto it = compute.rbegin(); it != compute.rend(); it++) {
        auto ctrl = it->first;
        auto tgt = it->second;
        if (leading_parent[tgt] && it->first != pivot) {
          mat[tgt] ^= mat[ctrl];
          uncompute.push_back(std::make_pair(ctrl, tgt));
        }
      }

      if (debug) {
        std::cout << "  Uncompute cycle:\n    ";
        for (auto [ctrl, tgt] : uncompute) std::cout << "CNOT " << ctrl << "," << tgt << "; ";
        std::cout << "\n";
      }


      leading_one[pivot] = i;
      ret.splice(ret.end(), swap);
      ret.splice(ret.end(), compute);
      ret.splice(ret.end(), uncompute);
    }
    return ret;
  }

}
}
