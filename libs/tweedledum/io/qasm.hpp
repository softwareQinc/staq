/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_lib.hpp"
#include "../networks/io_id.hpp"

#include <cassert>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <string>
#include <tweedledee/qasm/qasm.hpp>
#include <tweedledee/qasm/ast/visitor.hpp>

namespace tweedledum {

/*! \brief Reads OPENQASM 2.0 format
 */
void read_qasm_from_file(std::string const& path)
{
	auto program_ast = tweedledee::qasm::read_from_file(path);
	if (program_ast) {
		tweedledee::qasm::ast_printer printer;
		printer.visit(*program_ast);
	}
}

/*! \brief Writes network in OPENQASM 2.0 format into output stream
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
 * - `num_qubits`
 *
 * \param network A quantum network
 * \param os Output stream
 */
template<typename Network>
void write_qasm(Network const& network, std::ostream& os)
{
	// header
	os << "OPENQASM 2.0;\n";
	os << "include \"qelib1.inc\";\n";
	os << fmt::format("qreg q[{}];\n", network.num_qubits());
	if (network.num_cbits()) {
		os << fmt::format("creg c[{}];\n", network.num_cbits());
	}

	network.foreach_gate([&](auto const& node) {
		auto const& gate = node.gate;
		switch (gate.operation()) {
		default:
			std::cerr << "[w] unsupported gate type\n";
			assert(0);
			return true;

		case gate_lib::hadamard:
			gate.foreach_target([&](auto target) { os << fmt::format("h q[{}];\n", target); });
			break;

		case gate_lib::pauli_x:
			gate.foreach_target([&](auto target) { os << fmt::format("x q[{}];\n", target); });
			break;

		case gate_lib::pauli_z:
			gate.foreach_target([&](auto target) { os << fmt::format("z q[{}];\n", target); });
			break;

		case gate_lib::phase:
			gate.foreach_target([&](auto target) { os << fmt::format("s q[{}];\n", target); });
			break;

		case gate_lib::phase_dagger:
			gate.foreach_target([&](auto target) { os << fmt::format("sdg q[{}];\n", target); });
			break;

		case gate_lib::t:
			gate.foreach_target([&](auto target) { os << fmt::format("t q[{}];\n", target); });
			break;

		case gate_lib::t_dagger:
			gate.foreach_target([&](auto target) { os << fmt::format("tdg q[{}];\n", target); });
			break;

		case gate_lib::rotation_z:
			gate.foreach_target([&](auto target) {
				os << fmt::format("rz({}) q[{}];\n", gate.rotation_angle(), target);
			});
			break;

		case gate_lib::rotation_y:
			gate.foreach_target([&](auto target) {
				os << fmt::format("ry({}) q[{}];\n", gate.rotation_angle(), target);
			});
			break;

		case gate_lib::rotation_x:
			gate.foreach_target([&](auto target) {
				os << fmt::format("rx({}) q[{}];\n", gate.rotation_angle(), target);
			});
			break;

		case gate_lib::cx:
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("x q[{}];\n", control.index());
				}
				gate.foreach_target([&](auto target) {
					os << fmt::format("cx q[{}], q[{}];\n", control.index(), target);
				});
				if (control.is_complemented()) {
					os << fmt::format("x q[{}];\n", control.index());
				}
			});
			break;

		case gate_lib::swap: {
			std::vector<io_id> targets;
			gate.foreach_target([&](auto target) {
				targets.push_back(target);
			});
			os << fmt::format("cx q[{}], q[{}];\n", targets[0], targets[1]);
			os << fmt::format("cx q[{}], q[{}];\n", targets[1], targets[0]);
			os << fmt::format("cx q[{}], q[{}];\n", targets[0], targets[1]);
		} break;

		case gate_lib::mcx: {
			std::vector<io_id> controls;
			std::vector<io_id> targets;
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("x q[{}];\n", control.index());
				}
				controls.push_back(control.id()); 
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
				for (auto q : targets) {
					os << fmt::format("x q[{}];\n", q);
				}
				break;

			case 1u:
				for (auto q : targets) {
					os << fmt::format("cx q[{}],q[{}];\n", controls[0], q);
				}
				break;

			case 2u:
				for (auto i = 1u; i < targets.size(); ++i) {
					os << fmt::format("cx q[{}], q[{}];\n", targets[0],
					                   targets[i]);
				}
				os << fmt::format("ccx q[{}], q[{}], q[{}];\n", controls[0],
				                   controls[1], targets[0]);
				for (auto i = 1u; i < targets.size(); ++i) {
					os << fmt::format("cx q[{}], q[{}];\n", targets[0],
					                   targets[i]);
				}
				break;
			}
			gate.foreach_control([&](auto control) {
				if (control.is_complemented()) {
					os << fmt::format("x q[{}];\n", control.index());
				}
			});
		} break;
		}
		return true;
	});
}

/*! \brief Writes network in OPENQASM 2.0 format into a file
 *
 * **Required gate functions:**
 * - `foreach_control`
 * - `foreach_target`
 * - `op`
 * 
 * **Required network functions:**
 * - `num_qubits`
 * - `foreach_node`
 *
 * \param network A quantum network
 * \param filename Filename
 */
template<typename Network>
void write_qasm(Network const& network, std::string const& filename)
{
	std::ofstream os(filename.c_str(), std::ofstream::out);
	write_qasm(network, os);
}

} // namespace tweedledum
