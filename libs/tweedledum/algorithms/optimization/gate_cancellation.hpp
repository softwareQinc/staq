/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../generic/remove_marked.hpp"
#include <cassert>

namespace tweedledum {

/*! \brief Cancellation of consecutive adjoint gates.
 *
 * **Required network functions:**
 * - `add_gate`
 * - `foreach_gate`
 * - `foreach_child`
 * - `get_node`
 * - `visited`
 * - `set_visited`
 */
// TODO: still feels a bit hacky
template<typename Network>
Network gate_cancellation(Network const& network)
{
	using node_ptr = typename Network::node_type::pointer_type;
	uint32_t num_deletions = 0u;
	network.foreach_gate([&](auto& node) {
		std::vector<node_ptr> children;
		bool do_remove = false;
		network.foreach_child(node, [&](auto child_index) {
			if (!children.empty() && children.back() != child_index) {
				do_remove = false;
				return false;
			}
			children.push_back(child_index);
			auto& child = network.get_node(child_index);
			if (network.visited(child)) {
				return true;
			}
			if (node.gate.is_adjoint(child.gate)) {
				do_remove = true;
			}
			return true;
		});
		if (do_remove) {
			network.set_visited(node, 1);
			network.set_visited(network.get_node(children.back()), 1);
			num_deletions += 2;
			return;
		}
	});
	if (num_deletions == 0) {
		return network;
	}
	return remove_marked(network);
}

} // namespace tweedledum