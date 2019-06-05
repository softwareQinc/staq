/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../gates/gate_lib.hpp"
#include "../../gates/gate_base.hpp"
#include "../../networks/io_id.hpp"
#include "../generic/rewrite.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace tweedledum {
namespace detail {

// Barenco, A., Bennett, C.H., Cleve, R., DiVincenzo, D.P., Margolus, N., Shor, P., Sleator, T., Smolin,
// J.A. and Weinfurter, H., 1995. Elementary gates for quantum computation. Physical review A, 52(5), p.3457.
template<class Network>
void toffoli_barenco_decomposition(Network& network, std::vector<io_id> const& controls,
                                   io_id target, uint32_t controls_threshold)
{
	const auto num_controls = controls.size();
	assert(num_controls >= 2);

	if (num_controls <= controls_threshold) {
		network.add_gate(gate::mcx, controls, {target});
		return;
	}

	std::vector<io_id> workspace;
	network.foreach_qubit([&](io_id qid) {
		if (qid == target) {
			return;
		}
		for (auto control : controls) {
			if (qid.index() == control.index()) {
				return;
			}
		}
		workspace.push_back(qid);
	});
	const auto workspace_size = workspace.size();
	if (workspace_size == 0) {
		std::cout << "[e] no sufficient helper line found for mapping, break\n";
		return;
	}

	// Check if there are enough empty lines lines
	// Lemma 7.2: If n ≥ 5 and m ∈ {3, ..., ⌈n/2⌉} then a gate can be simulated by a network
	// consisting of 4(m − 2) gates.
	// n is the number of qubits
	// m is the number of controls
	if (network.num_qubits() + 1 >= (num_controls << 1)) {
		workspace.push_back(target);

		// When offset is equal to 0 this is computing the toffoli
		// When offset is 1 this is cleaning up the workspace, that is, restoring the state
		// to their initial state
		for (int offset = 0; offset <= 1; ++offset) {
			for (int i = offset; i < static_cast<int>(num_controls) - 2; ++i) {
				network.add_gate(gate::mcx,
				                 std::vector({controls[num_controls - 1 - i],
				                              workspace[workspace_size - 1 - i]}),
				                 std::vector({workspace[workspace_size - i]}));
			}

			network.add_gate(gate::mcx, std::vector({controls[0], controls[1]}),
			                 std::vector({workspace[workspace_size - (num_controls - 2)]}));

			for (int i = num_controls - 2 - 1; i >= offset; --i) {
				network.add_gate(gate::mcx,
				                 std::vector({controls[num_controls - 1 - i],
				                              workspace[workspace_size - 1 - i]}),
				                 std::vector({workspace[workspace_size - i]}));
			}
		}
		return;
	}

	// Not enough qubits in the workspace, extra decomposition step
	// Lemma 7.3: For any n ≥ 5, and m ∈ {2, ... , n − 3} a (n−2)-toffoli gate can be simulated
	// by a network consisting of two m-toffoli gates and two (n−m−1)-toffoli gates
	std::vector<io_id> controls0;
	std::vector<io_id> controls1;
	for (auto i = 0u; i < (num_controls >> 1); ++i) {
		controls0.push_back(controls[i]);
	}
	for (auto i = (num_controls >> 1); i < num_controls; ++i) {
		controls1.push_back(controls[i]);
	}
	auto free_qubit = workspace.front();
	controls1.push_back(free_qubit);
	toffoli_barenco_decomposition(network, controls0, free_qubit, controls_threshold);
	toffoli_barenco_decomposition(network, controls1, target, controls_threshold);
	toffoli_barenco_decomposition(network, controls0, free_qubit, controls_threshold);
	toffoli_barenco_decomposition(network, controls1, target, controls_threshold);
}

} /* namespace detail */

/*! \brief Parameters for `barenco_decomposition`. */
struct barenco_params {
	uint32_t controls_threshold = 2u;
};

/*! \brief Barenco decomposition.
 *
 * Decomposes all Multiple-controlled Toffoli gates with more than ``controls_threshold`` controls
 * into Toffoli gates with at most ``controls_threshold`` controls. This may introduce one
 * additional helper qubit called ancilla.
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
Network barenco_decomposition(Network const& network, barenco_params params = {})
{
	auto gate_rewriter = [&](auto& dest, auto const& gate) {
		if (gate.is(gate_lib::mcx)) {
			switch (gate.num_controls()) {
			case 0:
				gate.foreach_target([&](auto target) {
					dest.add_gate(gate::pauli_x, target);
				});
				break;

			case 1:
				gate.foreach_control([&](auto control) {
					gate.foreach_target([&](auto target) {
						dest.add_gate(gate::cx, control, target);
					});
				});
				break;

			default:
				std::vector<io_id> controls;
				std::vector<io_id> targets;
				gate.foreach_control([&](auto control) { controls.push_back(control); });
				gate.foreach_target([&](auto target) { targets.push_back(target); });
				for (auto i = 1u; i < targets.size(); ++i) {
					dest.add_gate(gate::cx, targets[0], targets[i]);
				}
				detail::toffoli_barenco_decomposition(dest, controls, targets[0],
				                                      params.controls_threshold);
				for (auto i = 1ull; i < targets.size(); ++i) {
					dest.add_gate(gate::cx, targets[0], targets[i]);
				}
				break;
			}
			return true;
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
