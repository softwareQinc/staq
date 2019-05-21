/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../traits.hpp"
#include "../gates/io3_gate.hpp"
#include "../utils/device.hpp"

#include <optional>
#include <vector>

namespace tweedledum {

/*! \brief
 *
 * **Required gate functions:**
 *
 * **Required network functions:**
 * 
 */
template<typename Network>
class mapping_view : public Network {
public:
	using gate_type = typename Network::gate_type;
	using node_type = typename Network::node_type;
	using node_ptr_type = typename Network::node_ptr_type;
	using storage_type = typename Network::storage_type;

	/*! \brief Default constructor.
	 *
	 * Constructs depth view on a network.
	 */
	explicit mapping_view(Network const& network, device const& arch, bool allow_partial = false)
	    : Network()
	    , init_virtual_phy_map_(arch.num_vertices)
	    , virtual_phy_map_(arch.num_vertices)
	    , coupling_matrix_(arch.get_coupling_matrix())
	    , allow_partial_(allow_partial)
	    , is_partial_(false)
	{
		assert(this->num_qubits() <= arch.num_vertices);
		std::iota(init_virtual_phy_map_.begin(), init_virtual_phy_map_.end(), 0u);
		std::iota(virtual_phy_map_.begin(), virtual_phy_map_.end(), 0u);
		network.foreach_io([&](io_id id, std::string const& label) {
			if (id.is_qubit()) {
				phy_id_map_.push_back(id);
				this->add_qubit(label);
			} else {
				this->add_cbit(label);
			}
		});
	}

#pragma region Add gates(qids)
	node_type& add_gate(gate_base op, io_id target)
	{
		auto const phy_target = virtual_phy_map_.at(target);
		return this->emplace_gate(gate_type(op, io_id(phy_target, true)));
	}

	std::optional<node_type> add_gate(gate_base op, io_id control, io_id target)
	{
		auto const phy_control = virtual_phy_map_.at(control);
		auto const phy_target = virtual_phy_map_.at(target);
		if (!coupling_matrix_.at(phy_control, phy_target)) {
			if (allow_partial_) {
				is_partial_ = true;
			} else {
				return std::nullopt;
			}
		}
		return this->emplace_gate(gate_type(op, phy_id_map_.at(phy_control), phy_id_map_.at(phy_target)));
	}
#pragma endregion

#pragma region Mapping
	/*! \brief Returns true if this is a partial mapping, i.e. the mapping is not valid */
	bool is_partial() const
	{
		return is_partial_;
	}

	/*! \brief Set virtual mapping (virtual qubit -> physical qubit). */
	void set_virtual_phy_map(std::vector<uint32_t> const& map)
	{
		if (this->num_gates() == 0) {
			init_virtual_phy_map_ = map;
		}
		virtual_phy_map_ = map;
	}

	std::vector<uint32_t> init_virtual_phy_map() const
	{
		return init_virtual_phy_map_;
	}

	/*! \brief Set physical mapping (physical qubit -> virtual qubit). */
	void phy_virtual_map(std::vector<uint32_t> const& map)
	{
		for (uint32_t i = 0; i < virtual_phy_map_.size(); ++i) {
			virtual_phy_map_[map[i]] = i;
		}
	}

	/*! \brief Returns physical mapping (physical qubit -> virtual qubit). */
	std::vector<uint32_t> phy_virtual_map() const 
	{
		std::vector<uint32_t> map(virtual_phy_map_.size(), 0);
		for (uint32_t i = 0; i < virtual_phy_map_.size(); ++i) {
			map[virtual_phy_map_[i]] = i;
		}
		return map;
	}

	/*! \brief Adds a SWAP gate between two physical qubits.
	 *  \param phy_a Physical qubit id
	 *  \param phy_b Physical qubit id
	 */
	void add_swap(uint32_t phy_a, uint32_t phy_b)
	{
		assert(coupling_matrix_.at(phy_a, phy_b));
		this->emplace_gate(gate_type(gate::swap, phy_id_map_.at(phy_a), phy_id_map_.at(phy_b)));
		auto it_a = std::find(virtual_phy_map_.begin(), virtual_phy_map_.end(), phy_a);
		auto it_b = std::find(virtual_phy_map_.begin(), virtual_phy_map_.end(), phy_b);
		std::swap(*it_a, *it_b);
	}
#pragma endregion

#pragma region Deleted methods
	node_type& add_gate(gate_base op, std::vector<io_id> controls,
	                    std::vector<io_id> targets) = delete;
	node_type& add_gate(gate_base op, std::string const& qlabel_target) = delete;
	node_type& add_gate(gate_base op, std::string const& qlabel_control,
	                    std::string const& qlabel_target) = delete;
	node_type& add_gate(gate_base op, std::vector<std::string> const& qlabels_control,
	                    std::vector<std::string> const& qlabels_target) = delete;
#pragma endregion

private:
	std::vector<io_id> phy_id_map_;
	std::vector<uint32_t> init_virtual_phy_map_;
	std::vector<uint32_t> virtual_phy_map_;
	bit_matrix_rm<> coupling_matrix_;
	bool allow_partial_;
	bool is_partial_;
};

} // namespace tweedledum
