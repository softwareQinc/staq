/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_set.hpp"

#include <fmt/format.h>
#include <fstream>
#include <iostream>

namespace tweedledum {

/*! \brief Writes network in qpic format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required gate functions:**
 * - `foreach_control`
 * - `foreach_target`
 * - `op`
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_qubit`
 * - `num_qubits`
 *
 * \param network A quantum network
 * \param os Output stream
 * \param color_marked_gates Flag to draw marked nodes in red
 */
template<typename Network>
void write_qpic(Network const& network, std::ostream& os, bool color_marked_gates = false)
{
	if (color_marked_gates) {
		os << "DEFINE mark color=red:style=thick\n";
	}
	network.foreach_io([&](auto id, auto const& name) {
		if (id.is_qubit()) {
			os << fmt::format("id{} W {} {}\n", id, name, name);
		} else {
			os << fmt::format("id{} W {} {} cwire\n", id, name, name);
		}
	});

	network.foreach_gate([&](auto& node) {
		auto prefix = "";
		if (node.gate.is(gate_set::mcx)) {
			prefix = "+";
		}
		node.gate.foreach_target([&](auto qubit) {
			os << fmt::format("{}id{} ", prefix, qubit);
		});
		switch (node.gate.operation()) {
		case gate_set::pauli_x:
			os << 'N';
			break;

		case gate_set::cx:
			os << 'C';
			break;

		case gate_set::mcx:
			break;

		case gate_set::pauli_z:
		case gate_set::cz:
		case gate_set::mcz:
			os << 'Z';
			break;

		case gate_set::hadamard:
			os << 'H';
			break;

		case gate_set::phase:
			os << "G $P$";
			break;

		case gate_set::phase_dagger:
			os << "G $P^{\\dagger}$";
			break;

		case gate_set::t:
			os << "G $T$";
			break;

		case gate_set::t_dagger:
			os << "G $T^{\\dagger}$";
			break;

		case gate_set::rotation_x:
			os << "G $R_{x}$";
			break;

		case gate_set::rotation_z:
			os << "G $R_{z}$";
			break;

		case gate_set::swap:
			os << "SWAP";
			break;

		default:
			break;
		}
		node.gate.foreach_control([&](auto qubit) {
			os << fmt::format(" {}id{}", qubit.is_complemented() ? "-" : "", qubit); 
		});
		os << '\n';
	});
}

/*! \brief Writes network in qpic format into a file
 *
 * **Required gate functions:**
 * - `foreach_control`
 * - `foreach_target`
 * - `op`
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_qubit`
 * - `num_qubits`
 *
 * \param network A quantum network
 * \param filename Filename
 * \param color_marked_gates Flag to draw marked nodes in red
 */
template<typename Network>
void write_qpic(Network const& network, std::string const& filename, bool color_marked_gates = false)
{
	std::ofstream os(filename.c_str(), std::ofstream::out);
	write_qpic(network, os, color_marked_gates);
}

} // namespace tweedledum
