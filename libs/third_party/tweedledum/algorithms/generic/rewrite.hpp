/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "shallow_duplicate.hpp"

#include <cassert>

namespace tweedledum {

/*! \brief Generic rewrite function.
 *
 * This function requires a template parameter that cannot be inferred. Useful
 * when duplicate into a different network type. Gate type must be the same
 * though.
 */
template <class NewNetwork, class Network, class RewriteFn>
NewNetwork rewrite_network(Network const& network, RewriteFn&& fn,
                           uint32_t ancillae = 0) {
    static_assert(std::is_same_v<typename Network::gate_type,
                                 typename NewNetwork::gate_type>);

    NewNetwork result = shallow_duplicate<NewNetwork>(network);
    for (uint32_t i = 0u; i < ancillae; ++i) {
        result.add_qubit();
    }
    network.foreach_gate([&](auto const& node) {
        if (!fn(result, node.gate)) {
            result.emplace_gate(node.gate);
        }
    });
    result.rewire(network.rewire_map());
}

/*! \brief Generic rewrite function. */
template <class Network, class RewriteFn>
Network rewrite_network(Network const& network, RewriteFn&& fn,
                        uint32_t ancillae = 0) {
    Network result = shallow_duplicate(network);
    for (auto i = 0u; i < ancillae; ++i) {
        result.add_qubit();
    }
    network.foreach_gate([&](auto const& node) {
        if (!fn(result, node.gate)) {
            result.emplace_gate(node.gate);
        }
    });
    result.rewire(network.rewire_map());
    return result;
}

} // namespace tweedledum
