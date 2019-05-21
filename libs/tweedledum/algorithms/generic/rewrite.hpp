/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <cassert>

namespace tweedledum {

/*! \brief Generic rewrite function.
 *
 * Useful when rewriting into a different network format. Gate type must be the same though
 * 
 * **Required network functions:**
 * - `add_gate`
 * - `foreach_qubit`
 * - `foreach_gate`
 * - `rewire`
 * - `rewire_map`
 */
template<class NetworkDest, class NetworkSrc, class RewriteFn>
void rewrite_network(NetworkDest& dest, NetworkSrc const& src, RewriteFn&& fn, uint32_t ancillae = 0)
{
	assert(dest.size() == 0);
	src.foreach_qubit([&](std::string const& label) {
		dest.add_qubit(label);
	});
	for (auto i = 0u; i < ancillae; ++i) {
		dest.add_qubit();
	}
	src.foreach_gate([&](auto const& node) {
		if (!fn(dest, node.gate)) {
			if constexpr (std::is_same_v<typename NetworkSrc::gate_type,
			                             typename NetworkDest::gate_type>) {
				dest.emplace_gate(node.gate);
			}
		}
	});
	dest.rewire(src.rewire_map());
}

/*! \brief Generic rewrite function.
 *
 * **Required network functions:**
 * - `add_gate`
 * - `foreach_qubit`
 * - `foreach_gate`
 * - `rewire`
 * - `rewire_map`
 */
template<class NetworkDest, class NetworkSrc, class RewriteFn>
NetworkDest rewrite_network(NetworkSrc const& src, RewriteFn&& fn, uint32_t ancillae = 0)
{
	NetworkDest dest;
	src.foreach_qubit([&](std::string const& qlabel) {
		dest.add_qubit(qlabel);
	});
	for (auto i = 0u; i < ancillae; ++i) {
		dest.add_qubit();
	}

	src.foreach_gate([&](auto const& node) {
		if (!fn(dest, node.gate)) {
			dest.add_gate(node.gate);
		}
	});
	dest.rewire(src.rewire_map());
	return dest;
}

} // namespace tweedledum
