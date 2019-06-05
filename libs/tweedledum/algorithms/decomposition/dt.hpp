/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../gates/gate_base.hpp"
#include "../../gates/gate_lib.hpp"
#include "../../networks/io_id.hpp"
#include "../generic/rewrite.hpp"
#include "gates/database.hpp"

#include <array>
#include <iostream>
#include <vector>

namespace tweedledum {

/*! \brief Parameters for `barenco_decomposition`. */
struct dt_params {
	bool use_t_par = false;
};

/*! \brief Direct Toffoli (DT) decomposition
 *
   \verbatim embed:rst

   Decomposes all Multiple-controlled Toffoli gates with 2, 3 or 4 controls into Clifford+T.
   Also decompose all Multiple-controlled Z gates with 2 controls into Clifford+T. This may
   introduce one additional helper qubit called ancilla.
   
   These Clifford+T represetations were obtained using techniques inspired by :cite:`Maslov2016`
   and given in :cite:`AAM13`

   \endverbatim
 * 
 * **Required gate functions:**
 * - `foreach_control`
 * - `foreach_target`
 * - `num_controls`
 *
 * **Required network functions:**
 * - `add_gate`
 * - `foreach_qubit`
 * - `foreach_gate`
 * - `rewire`
 * - `rewire_map`
 * 
 * \algtype decomposition
 * \algexpects A network
 * \algreturns A network
 */
template<typename Network>
Network dt_decomposition(Network const& network, dt_params params = {})
{
	auto gate_rewriter = [&](auto& dest, auto const& gate) {
		if (gate.is(gate_lib::mcx)) {
			std::array<io_id, 4> controls = {io_invalid, io_invalid, io_invalid, io_invalid};
			auto* p = controls.data();
			std::vector<io_id> targets;

			switch (gate.num_controls()) {
			default:
				return false;

			case 0u:
				gate.foreach_target([&](auto target) {
					dest.add_gate(gate::pauli_x, target);
				});
				break;

			case 1u:
				gate.foreach_control([&](auto control) {
					if (control.is_complemented()) {
						dest.add_gate(gate::pauli_x, !control);
					}
					gate.foreach_target([&](auto target) {
						dest.add_gate(gate::cx, control, target);
					});
					if (control.is_complemented()) {
						dest.add_gate(gate::pauli_x, !control);
					}
				});
				break;

			case 2u:
				gate.foreach_control([&](auto control) { *p++ = control; });
				gate.foreach_target([&](auto target) { targets.push_back(target); });
				if (!controls[0].is_complemented() && controls[1].is_complemented()) {
					std::swap(controls[0], controls[1]);
				}
				for (auto i = 1u; i < targets.size(); ++i) {
					dest.add_gate(gate::cx, targets[0], targets[i]);
				}
				detail::ccx(dest, controls[0], controls[1], targets[0], params.use_t_par);
				for (auto i = 1u; i < targets.size(); ++i) {
					dest.add_gate(gate::cx, targets[0], targets[i]);
				}
				break;

			case 3u:
				gate.foreach_control([&](auto control) {
					*p++ = control.id(); 
					if (control.is_complemented()) {
						dest.add_gate(gate::pauli_x, control);
					}
				});
				gate.foreach_target([&](auto target) { targets.push_back(target); });
				detail::cccx(dest, controls, targets);
				gate.foreach_control([&](auto control) {
					if (control.is_complemented()) {
						dest.add_gate(gate::pauli_x, control);
					}
				});
				break;

			case 4u:
				gate.foreach_control([&](auto control) {
					*p++ = control.id(); 
					if (control.is_complemented()) {
						dest.add_gate(gate::pauli_x, control);
						return;
					}
				});
				gate.foreach_target([&](auto target) { targets.push_back(target); });
				detail::ccccx(dest, controls, targets);
				gate.foreach_control([&](auto control) {
					if (control.is_complemented()) {
						dest.add_gate(gate::pauli_x, control);
					}
				});
				break;
			}
			return true;
		} else if (gate.is(gate_lib::mcz)) {
			if (gate.num_controls() == 2) {
				std::array<io_id, 2> controls = {io_invalid, io_invalid};
				auto* p = controls.data();
				std::vector<io_id> targets;

				gate.foreach_control([&](auto control) { *p++ = control; });
				gate.foreach_target([&](auto target) { targets.push_back(target); });
				assert(targets.size() == 1);

				if (!controls[0].is_complemented() && controls[1].is_complemented()) {
					std::swap(controls[0], controls[1]);
				}
				detail::ccz(dest, controls[0], controls[1], targets[0], params.use_t_par);
				return true;
			}
		}
		return false;
	};

	auto num_ancillae = 0u;
	network.foreach_gate([&](auto const& node) {
		if (node.gate.is(gate_lib::mcx) && node.gate.num_controls() > 2
		    && node.gate.num_controls() + 1 == network.num_qubits()) {
			num_ancillae = 1u;
			return false;
		}
		return true;
	});
	return rewrite_network(network, gate_rewriter, num_ancillae);
}

} // namespace tweedledum
