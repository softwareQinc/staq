/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../networks/io_id.hpp"
#include "../../utils/angle.hpp"
#include "../../utils/parity_terms.hpp"
#include "../../views/pathsum_view.hpp"
#include "../identify_rz.hpp"

#include <cstdint>
#include <fmt/format.h>
#include <iostream>
#include <vector>

namespace tweedledum {

struct phase_folding_params {
	bool use_generic_rx = false;
};

/*! \brief TODO
 *
 * **Required network functions:**
 */
template<typename Network>
Network phase_folding(Network const& network, phase_folding_params params = {})
{
	using term_type = typename pathsum_view<Network>::esop_type;
	Network result = shallow_duplicate(network);
	parity_terms<term_type> parities;

	pathsum_view pathsums(network);
	// Go thought the networ and merge angles of rotations being applied to the same pathsum.
	network.foreach_gate([&](auto const& node) {
		if (!node.gate.is_z_rotation()) {
			return;
		}
		auto term = pathsums.get_pathsum(node);
		parities.add_term(term, node.gate.rotation_angle());
	});

	network.foreach_gate([&](auto const& node) {
		if (node.gate.is_z_rotation()) {
			return;
		}
		result.emplace_gate(node.gate);
		auto angle = parities.extract_term(pathsums.get_pathsum(node));
		if (angle == angles::zero) {
			return;
		}
		result.add_gate(gate_base(gate_lib::rotation_z, angle), node.gate.target());
	});
	if (params.use_generic_rx == false) {
		result = identify_rz(result);
	}
	result.rewire(network.rewire_map());
	return result;
}

} // namespace tweedledum
