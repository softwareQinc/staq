/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Kate Smith
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_set.hpp"
#include "../networks/io_id.hpp"

#include <cassert>
#include <fmt/format.h>
#include <fstream>
#include <iostream>

namespace tweedledum {

/*! \brief Writes network in quil format into output stream
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
 */
template<typename Network>
void write_quil(Network const& network, std::ostream& os)
{
	network.foreach_gate([&](auto const& node) {
		auto const& gate = node.gate;
		switch (gate.operation()) {
		default:
			std::cerr << "[w] unsupported gate type\n";
			assert(0);
			return true;

		case gate_set::hadamard:
			gate.foreach_target([&](auto target) { os << fmt::format("H {}\n", target); });
			break;

		case gate_set::pauli_x:
			gate.foreach_target([&](auto target) { os << fmt::format("X {}\n", target); });
			break;

                case gate_set::pauli_z:
			gate.foreach_target([&](auto target) { os << fmt::format("Z {}\n", target); });
                        break;

		case gate_set::t:
			gate.foreach_target([&](auto target) { os << fmt::format("T {}\n", target); });
			break;

                case gate_set::phase:
			gate.foreach_target([&](auto target) { os << fmt::format("S {}\n", target); });
			break;

		case gate_set::t_dagger:
		case gate_set::phase_dagger:
		case gate_set::rotation_z:
			gate.foreach_target([&](auto target) {
				angle const& angle = gate.rotation_angle();
				os << fmt::format("RZ({}) {}\n", angle, target);
			});
			break;

		case gate_set::cx:
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("X {}\n", control.index());
				}
				gate.foreach_target([&](auto target) {
					os << fmt::format("CNOT {} {}\n", control.index(), target); 
				});
				if (control.is_complemented()) {
					os << fmt::format("X {}\n", control.index());
				}
			});
			break;
                
                case gate_set::cz:
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("X {}\n", control.index());
				}
				gate.foreach_target([&](auto target) {
					os << fmt::format("CZ {} {}\n", control.index(), target); 
				});
				if (control.is_complemented()) {
					os << fmt::format("X {}\n", control.index());
				}
			});
                        break;
		
		case gate_set::swap: {
			std::vector<io_id> targets;
			gate.foreach_target([&](auto target) {
				targets.push_back(target);
			});
			os << fmt::format("CNOT {} {}\n", targets[0], targets[1]);
			os << fmt::format("CNOT {} {}\n", targets[1], targets[0]);
			os << fmt::format("CNOT {} {}\n", targets[0], targets[1]);
		} break;

		case gate_set::mcx: {
			std::vector<io_id> controls;
			std::vector<io_id> targets;
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("X {}\n", control.index());
				}
				controls.emplace_back(control.id()); 
			});
			gate.foreach_target([&](auto target) {
				targets.push_back(target);
			});
			switch (controls.size()) {
			default:
				std::cerr << "[w] unsupported control size\n";
				assert(0);
				return true;

			case 0u:
				for (auto target : targets) {
					os << fmt::format("X {}\n", target);
				}
				break;

			case 1u:
				for (auto target : targets) {
					os << fmt::format("CNOT {} {}\n", controls[0], target);
				}
				break;

			case 2u:
				for (auto i = 1u; i < targets.size(); ++i) {
					os << fmt::format("CNOT {} {}\n", targets[0], targets[i]);
				}
				os << fmt::format("CCNOT {} {} {}\n", controls[0], controls[1],
				                   targets[0]);
				for (auto i = 1u; i < targets.size(); ++i) {
					os << fmt::format("CNOT {} {}\n", targets[0], targets[i]);
				}
				break;
			}
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("X {}\n", control.index());
				}
			});
		} break;
		}
		return true;
	});
}

/*! \brief Writes network in quil format into a file
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
 */
template<typename Network>
void write_quil(Network const& network, std::string const& filename)
{
	std::ofstream os(filename.c_str(), std::ofstream::out);
	write_quil(network, os);
}

} // namespace tweedledum
