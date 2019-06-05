/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_base.hpp"
#include "../networks/io_id.hpp"
#include "../utils/angle.hpp"
#include "generic/rewrite.hpp"

#include <array>
#include <cstdint>

namespace tweedledum {

/*! \brief This algorithm try to give identity to generic Rz gates. 
 *
 * **Required network functions:**
 */
template<typename Network>
Network identify_rz(Network const& network)
{
	auto gate_rewriter = [](auto& dest, auto const& gate) {
		if (!gate.is(gate_lib::rotation_z)) {
			return false;
		}
		angle rotation_angle = gate.rotation_angle();
		if (rotation_angle.is_numerically_defined()) {
			return false;
		}
		// Try to identify gates based on known rotations
		if (rotation_angle == angles::pi_quarter) {
			dest.add_gate(gate::t, gate.target());
			return true;
		} else if (rotation_angle == -angles::pi_quarter) {
			dest.add_gate(gate::t_dagger, gate.target());
			return true;
		} else if (rotation_angle == angles::pi_half) {
			dest.add_gate(gate::phase, gate.target());
			return true;
		} else if (rotation_angle == -angles::pi_half) {
			dest.add_gate(gate::phase_dagger, gate.target());
			return true;
		} else if (rotation_angle == angles::pi) {
			dest.add_gate(gate::pauli_z, gate.target());
			return true;
		}

		// Try to identify gates based composition known rotations
		// std::cout << "Try to compose!\n";
		auto [numerator, denominator] = rotation_angle.symbolic_value().value();
		if ((4 % denominator) != 0) {
			return false;
		}
		numerator *= (4 / denominator);
		if (numerator < 0) {
			numerator = 8 + numerator;
		}
		assert(numerator > 0 && numerator < 8);
		// Greedy solution
		// Values each gate (t, phase, pauli_z, phase_dagger, t_dagger)
		std::array<uint8_t, 5> values = {7, 6, 4, 2, 1};
		std::array<gate_base, 5> gates = {gate::t_dagger, gate::phase_dagger, gate::pauli_z,
		                                  gate::phase, gate::t};
		int32_t amount = numerator;
		for (uint32_t i = 0; i < values.size(); ++i) {
			while (amount >= values[i]) {
				amount -= values[i];
				dest.add_gate(gates[i], gate.target());
			}
		}
		assert(amount == 0);
		return true;
	};
	return rewrite_network(network, gate_rewriter);
}

} // namespace tweedledum
