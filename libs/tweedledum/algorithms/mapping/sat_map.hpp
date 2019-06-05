/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../utils/device.hpp"
#include "../../utils/sat/cardinality.hpp"
#include "../../utils/sat/solver.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/mapping_view.hpp"

namespace tweedledum {

#pragma region Implementation details
namespace detail {

template<typename Network, typename Cnf>
class map_cnf_encoder {
public:
	map_cnf_encoder(Network const& network, device const& device, Cnf& cnf_builder)
	    : network_(network)
	    , device_(device)
	    , pairs_(network_.num_qubits() * (network_.num_qubits() + 1) / 2, 0u)
	    , cnf_builder_(cnf_builder)
	    , io_qid_map_(network.num_io(), io_invalid)
	{
		network.foreach_io([&](io_id io) {
			if (io.is_qubit()) {
				io_qid_map_.at(io.index()) = qid_io_map_.size();
				qid_io_map_.push_back(io);
			}
		});
	}

	void encode()
	{
		cnf_builder_.add_variables(num_virtual_qubits() * num_physical_qubits());
		qubits_constraints();
		network_.foreach_gate([&](auto const& node) {
			if (!node.gate.is(gate_lib::cx)) {
				return;
			}
			uint32_t control = io_qid_map_.at(node.gate.control());
			uint32_t target = io_qid_map_.at(node.gate.target());
			if (pairs_[triangle_to_vector_idx(control, target)] == 0) {
				gate_constraints(control, target);
			}
			++pairs_[triangle_to_vector_idx(control, target)];
		});
		std::cout << fmt::format("Number of clauses: {}\n", cnf_builder_.num_clauses());
	}

	std::vector<uint32_t> decode(std::vector<sat::lbool_type> const& model)
	{
		std::vector<uint32_t> mapping;
		network_.foreach_io([&](io_id io) {
			mapping.push_back(io);
		});
		for (uint32_t i = mapping.size(); i < device_.num_vertices; ++i) {
			mapping.push_back(i);
		}

		for (uint32_t v_qid = 0; v_qid < num_virtual_qubits(); ++v_qid) {
			io_id v_id = qid_io_map_.at(v_qid);
			for (uint32_t phy_qid = 0; phy_qid < num_physical_qubits(); ++phy_qid) {
				sat::var_type var = virtual_physical_var(v_qid, phy_qid);
				if (model.at(var) == sat::lbool_type::true_) {
					mapping.at(v_id.index()) = phy_qid;
					break;
				}
			}
		}
		return mapping;
	}

private:
	uint32_t num_physical_qubits() const
	{
		return device_.num_vertices;
	}

	uint32_t num_virtual_qubits() const
	{
		return network_.num_qubits();
	}

	void qubits_constraints()
	{
		// Condition 2:
		std::vector<sat::var_type> variables;
		for (auto v = 0u; v < num_virtual_qubits(); ++v) {
			for (auto p = 0u; p < num_physical_qubits(); ++p) {
				variables.emplace_back(virtual_physical_var(v, p));
			}
			at_least_one(variables, cnf_builder_);
			at_most_one_pairwise(variables, cnf_builder_);
			variables.clear();
		}

		// Condition 3:
		for (auto p = 0u; p < num_physical_qubits(); ++p) {
			for (auto v = 0u; v < num_virtual_qubits(); ++v) {
				variables.emplace_back(virtual_physical_var(v, p));
			}
			at_most_one_pairwise(variables, cnf_builder_);
			variables.clear();
		}
	}

	// Abbreviations:
	// - c_v_qid (control, virtual qubit identifier)
	// - t_v_qid (target, virtual qubit identifier)
	// - c_phy_qid (control, physical qubit identifier)
	// - t_phy_qid (target, physical qubit identifier)
	void gate_constraints(uint32_t c_v_qid, uint32_t t_v_qid)
	{
		bit_matrix_rm<uint32_t> const& coupling_matrix = device_.get_coupling_matrix();
		std::vector<sat::lit_type> clause;
		for (uint32_t t_phy_qid = 0; t_phy_qid < num_physical_qubits(); ++t_phy_qid) {
			uint32_t t_v_phy_var = virtual_physical_var(t_v_qid, t_phy_qid);
			clause.emplace_back(t_v_phy_var, sat::negative_polarity);
			for (uint32_t c_phy_qid = 0; c_phy_qid < num_physical_qubits(); ++c_phy_qid) {
				if (c_phy_qid == t_phy_qid || !coupling_matrix.at(c_phy_qid, t_phy_qid)) {
					continue;
				}
				uint32_t c_v_phy_var = virtual_physical_var(c_v_qid, c_phy_qid);
				clause.emplace_back(c_v_phy_var, sat::positive_polarity);
			}
			cnf_builder_.add_clause(clause);
			clause.clear();
		}
	}

private:
	sat::var_type virtual_physical_var(uint32_t virtual_id, uint32_t physical_id)
	{
		return virtual_id * num_physical_qubits() + physical_id;
	}

	uint32_t triangle_to_vector_idx(uint32_t i, uint32_t j)
	{
		if (i > j) {
			std::swap(i, j);
		}
		return i * num_virtual_qubits() - (i - 1) * i / 2 + j - i;
	}

private:
	// Problem data
	Network const& network_;
	device const& device_;
	std::vector<uint32_t> pairs_;

	//
	Cnf& cnf_builder_;

	// Qubit normalization
	std::vector<uint32_t> io_qid_map_;
	std::vector<io_id> qid_io_map_;
};

template<typename Network>
std::vector<uint32_t> map_without_swaps(Network const& network, device const& device)
{
	sat::solver solver;
	map_cnf_encoder encoder(network, device, solver);
	encoder.encode();

	sat::result result = solver.solve();
	if (result) {
		std::cout << fmt::format("Found mapping!\n");
		return encoder.decode(result.model());
	}
	std::cout << fmt::format("Impossible to map without SWAPS\n");
	return {};
}

} // namespace detail
#pragma endregion

/*! \brief
 *
 * **Required gate functions:**
 *
 * **Required network functions:**
 *
 */
template<typename Network>
mapping_view<Network> sat_map(Network const& network, device const& device)
{
	mapping_view<Network> mapped_network(network, device);

	std::vector<uint32_t> mapping = detail::map_without_swaps(network, device);
	if (mapping.empty()) {
		return mapped_network;
	}
	mapped_network.set_virtual_phy_map(mapping);
	network.foreach_gate([&](auto const& node) {
		if (node.gate.is_single_qubit()) {
			mapped_network.add_gate(node.gate, node.gate.target());
			return;
		}
		mapped_network.add_gate(node.gate, node.gate.control(), node.gate.target());
	});

	return mapped_network;
}

} // namespace tweedledum
