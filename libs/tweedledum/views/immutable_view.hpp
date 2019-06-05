/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_base.hpp"
#include "../gates/gate_lib.hpp"

#include <string>
#include <vector>

namespace tweedledum {

/*! \brief Deletes all methods that can change the network.
 *
 * This view deletes all methods that can change the network structure such as
 * This view is convenient to use as a base class for other views that make some
 * computations based on the structure when being constructed.
 */
template<typename Network>
class immutable_view : public Network {
public:
	using gate_type = typename Network::gate_type;
	using node_type = typename Network::node_type;
	using node_ptr_type = typename Network::node_ptr_type;
	using storage_type = typename Network::storage_type;

	/*! \brief Default constructor.
	 *
	 * Constructs immutable view on a network.
	 */
	immutable_view(Network const& network)
	    : Network(network)
	{}

	auto add_qubit() = delete;
	auto add_qubit(std::string const&) = delete;
	auto& add_gate(gate_type const& gate) = delete;
	auto& add_gate(gate_base op, io_id target) = delete;
	auto& add_gate(gate_base op, io_id control, io_id target) = delete;
	auto& add_gate(gate_base op, std::vector<io_id> controls, std::vector<io_id> targets) = delete;
	auto& add_gate(gate_base op, std::string const& qlabel_target) = delete;
	auto& add_gate(gate_base op, std::string const& qlabel_control,
	               std::string const& qlabel_target) = delete;
	auto& add_gate(gate_base op, std::vector<std::string> const& qlabels_control,
	               std::vector<std::string> const& qlabels_target) = delete;
};

} // namespace tweedledum
