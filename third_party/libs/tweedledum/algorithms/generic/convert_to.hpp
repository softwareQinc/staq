/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "shallow_duplicate.hpp"

namespace tweedledum {

/*! \brief Convert a network to a new network type. (Gate type _must_ be the
 * same)
 */
template <class NewNetwork, class Network>
NewNetwork convert_to(Network const& network) {
    static_assert(std::is_same_v<typename Network::gate_type,
                                 typename NewNetwork::gate_type>);

    NewNetwork result = shallow_duplicate<NewNetwork>(network);
    network.foreach_gate(
        [&](auto const& node) { result.emplace_gate(node.gate); });
    result.rewire(network.rewire_map());
    return result;
}

} // namespace tweedledum
