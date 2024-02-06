/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2024 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * \file synthesis/linear_reversible.hpp
 * \brief Synthesis of CNOT circuits
 */

#ifndef SYNTHESIS_LINEARREVERSIBLE_HPP_
#define SYNTHESIS_LINEARREVERSIBLE_HPP_

#include <cstddef>
#include <list>
#include <vector>

#include "staq/mapping/device.hpp"

namespace staq {
namespace synthesis {

template <typename T>
using linear_op = std::vector<std::vector<T>>;

// TODO: replace vector<bool> with a custom packed bitvector that supports
// faster bitwise operations
inline std::vector<bool>& operator^=(std::vector<bool>& A,
                                     const std::vector<bool>& B) {
    for (std::size_t i = 0; i < A.size(); i++) {
        A[i] = B[i] ^ A[i];
    }
    return A;
}

static void print_linop(const linear_op<bool>& mat) {
    for (std::size_t i = 0; i < mat.size(); i++) {
        for (std::size_t j = 0; j < mat[i].size(); j++) {
            std::cout << (mat[i][j] ? "1" : "0");
        }
        std::cout << "\n";
    }
}

/**
 * \brief Linear reversible synthesis from Gauss-Jordan elimination
 */
static std::list<std::pair<int, int>> gauss_jordan(linear_op<bool> mat) {
    std::list<std::pair<int, int>> ret;

    if (mat.size() == 0) {
        return ret;
    }

    for (std::size_t i = 0; i < mat[0].size(); i++) {

        // Find pivot
        int pivot = -1;
        for (std::size_t j = i; j < mat.size(); j++) {
            if (mat[j][i] == true) {
                pivot = static_cast<int>(j);
                break;
            }
        }
        if (pivot == -1) {
            std::cerr << "Error: linear operator is not invertible\n";
            return ret;
        }

        // Swap into place
        if (pivot != i) {
            mat[pivot].swap(mat[i]);
            ret.emplace_back(pivot, static_cast<int>(i));
            ret.emplace_back(static_cast<int>(i), pivot);
            ret.emplace_back(pivot, static_cast<int>(i));
        }

        // Zero other rows
        for (std::size_t j = 0; j < mat.size(); j++) {
            if (j != i && mat[j][i] == true) {
                mat[j] ^= mat[i];
                ret.emplace_back(static_cast<int>(i), static_cast<int>(j));
            }
        }
    }

    ret.reverse();
    return ret;
}

/**
 * \brief Linear reversible synthesis from Gaussian elimination
 */
static std::list<std::pair<int, int>> gaussian_elim(linear_op<bool> mat) {
    std::list<std::pair<int, int>> ret;

    for (std::size_t i = 0; i < mat[0].size(); i++) {

        // Find pivot
        int pivot = -1;
        for (std::size_t j = i; j < mat.size(); j++) {
            if (mat[j][i] == true) {
                pivot = static_cast<int>(j);
                break;
            }
        }
        if (pivot == -1) {
            std::cerr << "Error: linear operator is not invertible\n";
            return ret;
        }

        // Swap into place
        if (pivot != i) {
            mat[pivot].swap(mat[i]);
            ret.emplace_back(pivot, static_cast<int>(i));
            ret.emplace_back(static_cast<int>(i), pivot);
            ret.emplace_back(pivot, static_cast<int>(i));
        }

        // Zero other row below diagonal
        for (std::size_t j = i + 1; j < mat.size(); j++) {
            if (mat[j][i] == true) {
                mat[j] ^= mat[i];
                ret.emplace_back(static_cast<int>(i), static_cast<int>(j));
            }
        }
    }

    for (int i = static_cast<int>(mat[0].size()) - 1; i >= 0; i--) {
        for (int j = i - 1; j >= 0; j--) {
            if (mat[j][i] == true) {
                mat[j] ^= mat[i];
                ret.emplace_back(static_cast<int>(i), static_cast<int>(j));
            }
        }
    }

    ret.reverse();
    return ret;
}

/**
 * \brief Steiner tree based device constrained CNOT synthesis
 *
 * Our version of steiner-gauss (see arXiv:1904.01972 and arXiv:1904.00633)
 * works a little differently from either of those. We follow arXiv:1904.00633
 * by filling the steiner points with 0's in the column in question, then
 * "flushing" the zeros with a reverse-topological order traversal.
 *
 * To deal with 1's to the left of the diagonal -- i.e. from paths that cross
 * the diagonal -- we adopt a different approach. In particular, the path
 * 2-->1-->0-->3 has the effect of adding 1's to the left of column 3 in the
 * following:
   \verbatim
   10100            10100             11010
   01000    fill    01110    flush    01000
   00110  ------->  00110  -------->  00110
   00101            00101             10001
   00010            00010             00010
   \endverbatim
 * Our solution is to keep track of the transitive dependencies on rows above
 * the diagonal. Then to uncompute 1's to the left of the diagonal we reverse
 * the sequence of CNOTs, restricted to just those CNOTs with targets in the
 * transitive dependencies. E.g.,
   \verbatim
   10100            10100             11010             10010
   01000    fill    01110    flush    01000  uncompute  01000
   00110  ------->  00110  -------->  00110  -------->  00110
   00101            00101             10001             00011
   00010            00010             00010             00010
   \endverbatim
 */
static std::list<std::pair<int, int>> steiner_gauss(linear_op<bool> mat,
                                                    mapping::Device& d) {
    std::list<std::pair<int, int>> ret;

    // Whether or not a row has a dependence on a row above the diagonal
    std::vector<bool> above_diagonal_dep(mat.size(), false);

    for (std::size_t i = 0; i < mat[0].size(); i++) {

        std::fill(above_diagonal_dep.begin(), above_diagonal_dep.end(), false);

        // Phase 0: Find a pivot
        int pivot = -1;
        int dist;
        for (std::size_t j = i; j < mat.size(); j++) {
            if (mat[j][i] == true) {
                if (pivot == -1 || d.distance(static_cast<int>(j),
                                              static_cast<int>(i)) < dist) {
                    pivot = static_cast<int>(j);
                    dist = d.distance(static_cast<int>(j), static_cast<int>(i));
                }
            }
        }
        if (pivot == -1) {
            std::cerr << "Error: linear operator is not invertible\n";
            return ret;
        }

        std::list<std::pair<int, int>> swap;
        std::list<std::pair<int, int>> uncompute_swap;
        bool crossed_diag = false;
        auto path = d.shortest_path(pivot, static_cast<int>(i));
        std::size_t ctrl = pivot;

        // Phase 1: Fill 1's in column i along shortest path to row i
        for (auto tgt : path) {
            if (tgt != ctrl && mat[tgt][i] == false) {
                mat[tgt] ^= mat[ctrl];
                swap.emplace_back(static_cast<int>(ctrl),
                                  static_cast<int>(tgt));
                if (ctrl < i) {
                    crossed_diag = true;
                }
                above_diagonal_dep[tgt] = above_diagonal_dep[tgt] ||
                                          above_diagonal_dep[ctrl] ||
                                          (ctrl < i);
            }

            ctrl = tgt;
        }

        // Phase 2: If the path crossed the diagonal, corrections needed
        if (crossed_diag) {
            // First zero out column i along the path
            auto tgt = i;
            for (auto it = std::next(path.rbegin()); it != path.rend(); it++) {
                std::size_t ctrl = *it;
                if (tgt != i) {
                    mat[tgt] ^= mat[ctrl];
                    swap.emplace_back(static_cast<int>(ctrl),
                                      static_cast<int>(tgt));
                    above_diagonal_dep[tgt] = above_diagonal_dep[tgt] ||
                                              above_diagonal_dep[ctrl] ||
                                              (ctrl < i);
                }
                tgt = ctrl;
            }

            // Now repeat the computation to remove above diagonals
            for (auto it = swap.rbegin(); it != swap.rend(); it++) {
                auto ctrl = it->first;
                auto tgt = it->second;
                if (above_diagonal_dep[tgt] && it->first != pivot) {
                    mat[tgt] ^= mat[ctrl];
                    uncompute_swap.emplace_back(static_cast<int>(ctrl),
                                                static_cast<int>(tgt));
                }
            }
        }

        // Our pivot is now necessarily row i
        pivot = static_cast<int>(i);
        std::fill(above_diagonal_dep.begin(), above_diagonal_dep.end(), false);

        // Phase 3: Compute steiner tree covering the 1's in column i
        std::list<int> pivots;
        for (std::size_t j = 0; j < mat.size(); j++) {
            if (j != i && mat[j][i] == true) {
                pivots.push_back(static_cast<int>(j));
            }
        }
        auto s_tree = d.steiner(pivots, pivot);

        std::list<std::pair<int, int>> compute;
        // Phase 4: Propagate 1's to column i for each Steiner point
        for (auto& [ctrl, tgt] : s_tree) {
            if (mat[tgt][i] == false) {
                mat[tgt] ^= mat[ctrl];
                compute.emplace_back(ctrl, tgt);

                above_diagonal_dep[tgt] = above_diagonal_dep[tgt] ||
                                          above_diagonal_dep[ctrl] ||
                                          (ctrl < pivot);
            }
        }

        // Phase 5: Empty all 1's from column i in the Steiner tree
        for (auto it = s_tree.rbegin(); it != s_tree.rend(); it++) {
            auto ctrl = it->first;
            auto tgt = it->second;

            mat[tgt] ^= mat[ctrl];
            compute.emplace_back(ctrl, tgt);

            above_diagonal_dep[tgt] = above_diagonal_dep[tgt] ||
                                      above_diagonal_dep[ctrl] ||
                                      (ctrl < pivot);
        }

        // Phase 6: For each node that has an above diagonal dependency,
        // reverse the previous steps to undo the additions
        std::list<std::pair<int, int>> uncompute;
        for (auto it = compute.rbegin(); it != compute.rend(); it++) {
            auto ctrl = it->first;
            auto tgt = it->second;
            if (above_diagonal_dep[tgt] && it->first != pivot) {
                mat[tgt] ^= mat[ctrl];
                uncompute.emplace_back(ctrl, tgt);
            }
        }

        ret.splice(ret.end(), swap);
        ret.splice(ret.end(), uncompute_swap);
        ret.splice(ret.end(), compute);
        ret.splice(ret.end(), uncompute);
    }

    ret.reverse();
    return ret;
}

} /* namespace synthesis */
} /* namespace staq */

#endif /* SYNTHESIS_LINEARREVERSIBLE_HPP_ */
