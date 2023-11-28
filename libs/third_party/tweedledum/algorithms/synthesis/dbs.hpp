/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../networks/netlist.hpp"
#include "../../networks/io_id.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fmt/format.h>
#include <iostream>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/esop.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>
#include <list>
#include <vector>

namespace tweedledum {

/*! \brief Parameters for `dbs`. */
struct dbs_params {
    /*! \brief Be verbose. */
    bool verbose = false;
};

namespace detail {

inline auto decompose(std::vector<uint32_t>& permutation, uint8_t var) {
    std::vector<uint32_t> left(permutation.size(), 0);
    std::vector<uint32_t> right(permutation.size(), 0);
    std::vector<uint8_t> visited(permutation.size(), 0);

    uint32_t row = 0u;
    while (true) {
        if (visited[row]) {
            const auto it = std::find(visited.begin(), visited.end(), 0);
            if (it == visited.end()) {
                break;
            }
            row = std::distance(visited.begin(), it);
        }

        /* assign 0 to var on left side */
        left[row] = (row & ~(1 << var));
        visited[row] = 1;

        /* assign 1 to var on left side */
        left[row ^ (1 << var)] = left[row] ^ (1 << var);
        row ^= (1 << var);
        visited[row] = 1;

        /* assign 1 to var on right side */
        right[permutation[row] | (1 << var)] = permutation[row];

        /* assign 0 to var on left side */
        right[permutation[row] & ~(1 << var)] = permutation[row] ^ (1 << var);

        row = std::distance(permutation.begin(),
                            std::find(permutation.begin(), permutation.end(),
                                      permutation[row] ^ (1 << var)));
    }

    std::vector<uint32_t> perm_old = permutation;
    for (uint32_t row = 0; row < permutation.size(); ++row) {
        permutation[left[row]] = right[perm_old[row]];
    }

    return std::make_pair(left, right);
}

inline auto control_function_abs(uint32_t num_vars,
                                 std::vector<uint32_t> const& permutation) {
    kitty::dynamic_truth_table tt(num_vars);
    for (uint32_t row = 0; row < permutation.size(); ++row) {
        if (permutation[row] != row) {
            kitty::set_bit(tt, row);
        }
    }

    std::vector<io_id> base;
    for (auto element : kitty::min_base_inplace(tt)) {
        base.emplace_back(element, true);
    }
    return std::make_pair(kitty::shrink_to(tt, base.size()), base);
}

} // namespace detail

/*! \brief Reversible synthesis based on functional decomposition.
 *
   \verbatim embed:rst
   This algorithm implements the decomposition-based synthesis algorithm
 proposed in :cite:`VR08`. A permutation is specified as a vector of :math:`2^n`
 different integers ranging from :math:`0` to :math:`2^n-1`.

   .. code-block:: c++

      std::vector<uint32_t> permutation{{0, 2, 3, 5, 7, 1, 4, 6}};
      auto network = dbs<netlist<io3_gate>>(permutation, stg_from_spectrum());

   \endverbatim
 *
 * \param permutation A vector of different integers
 * \param stg_synth Synthesis function for single-target gates
 * \param params Parameters (see ``dbs_params``)
 *
 * \algtype synthesis
 * \algexpects Permutation
 * \algreturns Quantum or reversible circuit
 */
template <class Network, class STGSynthesisFn>
Network dbs(std::vector<uint32_t> permutation, STGSynthesisFn&& stg_synth,
            dbs_params params = {}) {
    Network network;
    const uint32_t num_qubits = std::log2(permutation.size());
    for (auto i = 0u; i < num_qubits; ++i) {
        network.add_qubit();
    }

    std::list<std::pair<kitty::dynamic_truth_table, std::vector<io_id>>> gates;
    auto pos = gates.begin();
    for (uint32_t i = 0u; i < num_qubits; ++i) {
        const auto [left, right] = detail::decompose(permutation, i);

        auto [tt_l, vars_l] = detail::control_function_abs(num_qubits, left);
        vars_l.emplace_back(i, true);

        auto [tt_r, vars_r] = detail::control_function_abs(num_qubits, right);
        vars_r.emplace_back(i, true);

        // TODO merge middle gates
        if (!kitty::is_const0(tt_l)) {
            pos = gates.emplace(pos, tt_l, vars_l);
            pos++;
        }
        if (!kitty::is_const0(tt_r)) {
            pos = gates.emplace(pos, tt_r, vars_r);
        }
    }

    for (auto const& [tt, vars] : gates) {
        if (params.verbose) {
            std::cout << "[i] synthesize " << kitty::to_hex(tt) << " onto "
                      << std::accumulate(vars.begin() + 1, vars.end(),
                                         std::to_string(vars.front()),
                                         [](auto const& a, auto v) {
                                             return a + ", " +
                                                    std::to_string(v);
                                         })
                      << "\n";
        }
        stg_synth(network, vars, tt);
    }
    return network;
}

} // namespace tweedledum