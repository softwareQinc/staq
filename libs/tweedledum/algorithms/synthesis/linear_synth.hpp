/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Fereshte Mozafari
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../gates/gate_lib.hpp"
#include "../../gates/gate_base.hpp"
#include "../../networks/io_id.hpp"
#include "../../utils/parity_terms.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <tuple>
#include <vector>

namespace tweedledum {
namespace detail {

template<class Network>
void linear_synth_binary(Network& network, std::vector<io_id> const& qubits, parity_terms<uint32_t> parities)
{
	const auto num_qubits = qubits.size();

	// Initialize the parity of each qubit state
	// Applying phase gate to parities that consisting of just one variable
	// i is the index of the target
	std::vector<uint32_t> qubits_states;
	for (auto i = 0u; i < num_qubits; ++i) {
		qubits_states.emplace_back((1u << i));
		auto rotation_angle = parities.extract_term(qubits_states[i]);
		if (rotation_angle != 0.0) {
			network.add_gate(gate_base(gate_lib::rotation_z, rotation_angle), qubits[i]);
		}
	}

	// Synthesize the network
	for (auto i = 1u; i < (1u << num_qubits); i++) {
		if ((i ^ (1 << (i - 1))) == 0) {
			continue;
		}
		uint32_t first_num = std::floor(std::log2(i));
		for (auto j = 0u; j < num_qubits; j++) {
			if ((first_num != j) && ((qubits_states[j] ^ qubits_states[first_num]) == i)) {
				qubits_states[first_num] ^= qubits_states[j];
				network.add_gate(gate::cx, qubits[j], qubits[first_num]);
				auto rotation_angle = parities.extract_term(qubits_states[first_num]);
				if (rotation_angle != 0.0) {
					network.add_gate(gate_base(gate_lib::rotation_z,
					                           rotation_angle),
					                 qubits[first_num]);
				}
			}
		}
	}

	// Return qubits to initial state
	for (int i = num_qubits - 1; i > 0; i--) {
		network.add_gate(gate::cx, qubits[i - 1], qubits[i]);
	}
}

template<class Network>
void linear_synth_gray(Network& network, std::vector<io_id> const& qubits, parity_terms<uint32_t> parities)
{
	const auto num_qubits = qubits.size();

	// Generate Gray code
	std::vector<uint32_t> gray_code;
	for (auto i = 0u; i < (1u << num_qubits); ++i) {
		gray_code.emplace_back((i >> 1) ^ i);
	}

	// Initialize the parity of each qubit state
	// Applying phase gate to parities that consisting of just one variable
	// i is the index of the target
	std::vector<uint32_t> qubits_states;
	for (auto i = 0u; i < num_qubits; ++i) {
		qubits_states.emplace_back((1u << i));
		auto rotation_angle = parities.extract_term(qubits_states[i]);
		if (rotation_angle != 0.0) {
			network.add_gate(gate_base(gate_lib::rotation_z, rotation_angle), qubits[i]);
		}
	}

	// Synthesize the network
	// i is the index of the target
	for (auto i = num_qubits - 1u; i > 0; --i) {
		for (auto j = (1u << (i + 1)) - 1u; j > (1u << i); --j) {
			const auto temp = std::log2(gray_code[j] ^ gray_code[j - 1u]);
			qubits_states[i] ^= qubits_states[temp];
			network.add_gate(gate::cx, qubits[temp], qubits[i]);
			auto rotation_angle = parities.extract_term(qubits_states[i]);
			if (rotation_angle != 0.0) {
				network.add_gate(gate_base(gate_lib::rotation_z, rotation_angle),
				                 qubits[i]);
			}
		}
		const auto temp = std::log2(gray_code[1 << i] ^ gray_code[(1u << (i + 1)) - 1u]);
		qubits_states[i] ^= qubits_states[temp];
		network.add_gate(gate::cx, qubits[temp], qubits[i]);
		auto rotation_angle = parities.extract_term(qubits_states[i]);
		if (rotation_angle != 0.0) {
			network.add_gate(gate_base(gate_lib::rotation_z, rotation_angle), qubits[i]);
		}
	}
}

} // namespace detail

/*! \brief Parameters for `linear_synth`. */
struct linear_synth_params {
	enum class strategy : uint8_t {
		binary,
		gray,
	} strategy = strategy::gray;
};

/*! \brief Linear synthesis for small {CNOT, Rz} networks
 *
 * This is the in-place variant of ``linear_synth``, in which the network is passed as a parameter
 * and can potentially already contain some gates. The parameter ``qubits`` provides a qubit
 * mapping to the existing qubits in the network.
 * 
 * \param network  A quantum network
 * \param qubits   The subset of qubits the linear reversible circuit acts upon
 * \param parities List of parities and rotation angles to synthesize
 * \param params   The parameters that configure the synthesis process.
 *                 See `linear_synth_params` for details.
 */
template<class Network>
void linear_synth(Network& network, std::vector<io_id> const& qubits,
                  parity_terms<uint32_t> const& parities, linear_synth_params params = {})
{
	assert(qubits.size() <= 6);
	switch (params.strategy) {
		case linear_synth_params::strategy::binary:
			detail::linear_synth_binary(network, qubits, parities);
			break;
		case linear_synth_params::strategy::gray:
			detail::linear_synth_gray(network, qubits, parities);
			break;
	}
}

/*! \brief Linear synthesis for small {CNOT, Rz} networks.
 *
 * Synthesize all linear combinations.
 *
 * \param num_qubits Number of qubits
 * \param parities   List of parities and rotation angles to synthesize
 * \param params     The parameters that configure the synthesis process.
 *                   See `linear_synth_params` for details.
 * 
 * \return {CNOT, Rz} network
 *
 * \algtype synthesis
 * \algexpects List of parities and rotation angles to synthesize
 * \algreturns {CNOT, Rz} network
 */
template<class Network>
Network linear_synth(uint32_t num_qubits, parity_terms<uint32_t> const& parities,
                     linear_synth_params params = {})
{
	assert(num_qubits <= 6);
	Network network;
	for (auto i = 0u; i < num_qubits; ++i) {
		network.add_qubit();
	}
	linear_synth(network, network.rewire_map(), parities, params);
	return network;
}

} // namespace tweedledum
