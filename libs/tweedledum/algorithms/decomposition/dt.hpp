/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../gates/gate_set.hpp"
#include "../../gates/gate_base.hpp"
#include "../../networks/io_id.hpp"
#include "../generic/rewrite.hpp"

#include <array>
#include <iostream>
#include <vector>

namespace tweedledum {
namespace detail {

template<typename Network>
void ccx(Network& network, std::array<io_id, 4> const& controls, std::vector<io_id> const& targets)
{
	const auto target = targets[0];
	for (auto i = 1u; i < targets.size(); ++i) {
		network.add_gate(gate::cx, target, targets[i]);
	}
	network.add_gate(gate::hadamard, target);

	network.add_gate(gate::cx, controls[1].id(), target);
	network.add_gate(controls[0].is_complemented() ? gate::t : gate::t_dagger, target);
	network.add_gate(gate::cx, controls[0].id(), target);
	network.add_gate(gate::t, target);
	network.add_gate(gate::cx, controls[1].id(), target);
	network.add_gate(controls[1].is_complemented() ? gate::t : gate::t_dagger, target);
	network.add_gate(gate::cx, controls[0].id(), target);
	network.add_gate(controls[0].is_complemented() && !controls[1].is_complemented()? gate::t_dagger : gate::t, target);
	
	network.add_gate(gate::cx, controls[0].id(), controls[1]);
	network.add_gate(gate::t_dagger, controls[1]);
	network.add_gate(gate::cx, controls[0].id(), controls[1]);
	network.add_gate(controls[1].is_complemented() ? gate::t_dagger : gate::t, controls[0]);
	network.add_gate(controls[0].is_complemented() ? gate::t_dagger : gate::t, controls[1]);

	network.add_gate(gate::hadamard, target);
	for (auto i = 1u; i < targets.size(); ++i) {
		network.add_gate(gate::cx, target, targets[i]);
	}
}

template<typename Network>
void cccx(Network& network, std::array<io_id, 4> const& controls, std::vector<io_id> const& targets)
{
	const auto a = controls[0];
	const auto b = controls[1];
	const auto c = controls[2];
	const auto target = targets[0];

	// Find helper qubit
	auto helper = network.foreach_qubit([&](io_id qid) -> bool {
		if (qid == a || qid == b || qid == c) {
			return true;
		}
		for (auto t : targets) {
			if (qid == t) {
				return true;
			}
		}
		// will return the current qid
		return false;
	});
	assert(helper != io_invalid);

	for (auto i = 1u; i < targets.size(); ++i) {
		network.add_gate(gate::cx, target, targets[i]);
	}

	// R1-TOF(a, b, helper)
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, a, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);

	// S-R2-TOF(c, helper, target)
	network.add_gate(gate::hadamard, target);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t, helper);

	// R1-TOF^-1(a, b, helper)
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, a, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);

	// S-R2-TOF^-1(c, helper, target)
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::hadamard, target);

	for (auto i = 1u; i < targets.size(); ++i) {
		network.add_gate(gate::cx, target, targets[i]);
	}
}

template<typename Network>
void ccccx(Network& network, std::array<io_id, 4> const& controls, std::vector<io_id> const& targets)
{
	const auto a = controls[0];
	const auto b = controls[1];
	const auto c = controls[2];
	const auto d = controls[3];
	const auto target = targets[0];

	// Find helper qubit
	auto helper = network.foreach_qubit([&](io_id qid) -> bool {
		if (qid == a || qid == b || qid == c || qid == d) {
			return true;
		}
		for (auto t : targets) {
			if (qid == t) {
				return true;
			}
		}
		// will return the current qid
		return false;
	});
	assert(helper != io_invalid);

	for (auto i = 1u; i < targets.size(); ++i) {
		network.add_gate(gate::cx, target, targets[i]);
	}

	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::cx, a, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, a, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::hadamard, target);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, d, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, d, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, a, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, b, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, a, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, c, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::hadamard, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, d, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::t_dagger, helper);
	network.add_gate(gate::cx, d, helper);
	network.add_gate(gate::t, helper);
	network.add_gate(gate::cx, target, helper);
	network.add_gate(gate::hadamard, target);

	for (auto i = 1u; i < targets.size(); ++i) {
		network.add_gate(gate::cx, target, targets[i]);
	}
}

template<typename Network>
void ccz(Network& network, std::array<io_id, 2> const& controls, io_id target)
{
	network.add_gate(gate::cx, controls[1].id(), target);
	network.add_gate(controls[0].is_complemented() ? gate::t : gate::t_dagger, target);
	network.add_gate(gate::cx, controls[0].id(), target);
	network.add_gate(gate::t, target);
	network.add_gate(gate::cx, controls[1].id(), target);
	network.add_gate(controls[1].is_complemented() ? gate::t : gate::t_dagger, target);
	network.add_gate(gate::cx, controls[0].id(), target);
	network.add_gate(controls[0].is_complemented() && !controls[1].is_complemented()? gate::t_dagger : gate::t, target);
	
	network.add_gate(gate::cx, controls[0].id(), controls[1]);
	network.add_gate(gate::t_dagger, controls[1]);
	network.add_gate(gate::cx, controls[0].id(), controls[1]);
	network.add_gate(controls[1].is_complemented() ? gate::t_dagger : gate::t, controls[0]);
	network.add_gate(controls[0].is_complemented() ? gate::t_dagger : gate::t, controls[1]);
}

} // namespace detail

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
Network dt_decomposition(Network const& src)
{
	auto gate_rewriter = [](auto& dest, auto const& gate) {
		if (gate.is(gate_set::mcx)) {
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
				detail::ccx(dest, controls, targets);
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
		} else if (gate.is(gate_set::mcz)) {
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
				detail::ccz(dest, controls, targets[0]);
				return true;
			}
		}
		return false;
	};

	auto num_ancillae = 0u;
	src.foreach_gate([&](auto const& node) {
		if (node.gate.is(gate_set::mcx) && node.gate.num_controls() > 2
		    && node.gate.num_controls() + 1 == src.num_qubits()) {
			num_ancillae = 1u;
			return false;
		}
		return true;
	});
	Network dest;
	rewrite_network(dest, src, gate_rewriter, num_ancillae);
	return dest;
}

} // namespace tweedledum
