/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "shallow_duplicate.hpp"

namespace tweedledum {

/*! \brief Generic function to remove marked gates.
 *
 * This function requires a template parameter that cannot be inferred. Useful when duplicate into a
 * different network format. Gate type must be the same though.
*/
template<class NewNetwork, class Network>
NewNetwork rewrite_network(Network const& network)
{
	static_assert(std::is_same_v<typename Network::gate_type, typename NewNetwork::gate_type>);

	NewNetwork result = shallow_duplicate<NewNetwork>(network);
	network.foreach_gate([&](auto const& node) {
		if (network.visited(node)) {
			return;
		}
		result.emplace_gate(node.gate);
	});
	return result.rewire(network.rewire_map());
}

/*! \brief Generic function to remove marked gates. */
template<class Network>
Network remove_marked(Network const& network)
{
	Network result = shallow_duplicate(network);
	network.foreach_gate([&](auto const& node) {
		if (network.visited(node)) {
			return;
		}
		result.emplace_gate(node.gate);
	});
	result.rewire(network.rewire_map());
	return result;
}

} // namespace tweedledum
