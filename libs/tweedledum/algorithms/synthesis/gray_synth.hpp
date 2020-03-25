/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Fereshte Mozafari
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../gates/gate_lib.hpp"
#include "../../networks/io_id.hpp"
#include "../../utils/bit_matrix_cm.hpp"
#include "../../utils/bit_matrix_rm.hpp"
#include "../../utils/dynamic_bitset.hpp"
#include "../../utils/parity_terms.hpp"
#include "cnot_patel.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <tuple>
#include <vector>

namespace tweedledum {

/*! \brief Parameters for `gray_synth`. */
struct gray_synth_params {
    cnot_patel_params cp_params = {
        /*allow_rewiring*/ true,
        /*best_partition_size*/ true};
};

namespace detail {

template <class Network>
class gray_synth_ftor {
    using matrix_type = bit_matrix_cm<uint32_t>;
    using qubit_pair_type = std::pair<uint32_t, uint32_t>;

    struct state_type {
        std::vector<uint32_t> selected_columns;
        dynamic_bitset<uint32_t> remaining_rows;
        uint32_t qid;
    };

  public:
    gray_synth_ftor(Network& network, std::vector<io_id> const& qubits,
                    parity_terms<uint32_t> const& parities,
                    gray_synth_params params)
        : network_(network), qubits_(qubits), parities_(parities),
          parity_matrix_(num_qubits()), parameters_(params) {
        for (auto const& [term, angle] : parities) {
            parity_matrix_.emplace_back_column(term);
            (void) angle;
        }
        std::vector<uint32_t> selected_columns(parity_matrix_.num_columns());
        std::iota(selected_columns.begin(), selected_columns.end(), 0u);
        state_type initial_state = {
            selected_columns, dynamic_bitset<uint32_t>(num_qubits(), true),
            num_qubits()};
        state_stack_.emplace_back(std::move(initial_state));
    }

    void synthesize() {
        std::vector<qubit_pair_type> gates;

        while (!state_stack_.empty()) {
            auto state = std::move(state_stack_.back());
            state_stack_.pop_back();

            if (state.qid != num_qubits()) {
                for (auto j = 0u; j < num_qubits(); ++j) {
                    if (j == state.qid) {
                        continue;
                    }
                    if (!parity_matrix_.row(state.selected_columns, j).all()) {
                        continue;
                    }
                    parity_matrix_.foreach_column(
                        [&](auto& column) { column[j] ^= column[state.qid]; });
                    gates.emplace_back(j, state.qid);
                }
            }

            if (state.selected_columns.size() == 1 &&
                parity_matrix_.column(state.selected_columns[0]).count() <= 1) {
                continue;
            }
            if (state.remaining_rows.count() == 0) {
                continue;
            }

            auto row_index =
                select_row(state.selected_columns, state.remaining_rows);
            std::vector<uint32_t> cofactor0;
            std::vector<uint32_t> cofactor1;
            parity_matrix_.foreach_column(
                state.selected_columns, [&](auto& column, auto column_index) {
                    if (column[row_index] == 1) {
                        cofactor1.push_back(column_index);
                        return;
                    }
                    cofactor0.push_back(column_index);
                });

            state.remaining_rows[row_index] = 0;
            if (!cofactor1.empty()) {
                state.selected_columns = std::move(cofactor1);
                state_stack_.emplace_back(state);
                state_stack_.back().qid =
                    (state.qid == num_qubits()) ? row_index : state.qid;
            }
            if (!cofactor0.empty()) {
                state.selected_columns = std::move(cofactor0);
                state_stack_.emplace_back(state);
                state_stack_.back().qid = state.qid;
            }
        }

        // Initialize the parity of each qubit state
        // Applying phase gate to parities that consisting of just one variable
        // i is the index of the target
        std::vector<uint32_t> qubits_states;
        for (auto i = 0u; i < num_qubits(); ++i) {
            qubits_states.emplace_back((1u << i));
            auto rotation_angle = parities_.extract_term(qubits_states[i]);
            if (rotation_angle != 0.0) {
                network_.add_gate(
                    gate_base(gate_lib::rotation_z, rotation_angle),
                    qubits_[i]);
            }
        }

        for (const auto [control, target] : gates) {
            qubits_states[target] ^= qubits_states[control];
            network_.add_gate(gate::cx, qubits_[control], qubits_[target]);
            auto rotation_angle = parities_.extract_term(qubits_states[target]);
            if (rotation_angle != 0.0) {
                network_.add_gate(
                    gate_base(gate_lib::rotation_z, rotation_angle),
                    qubits_[target]);
            }
        }

        // Finilize
        bit_matrix_rm transformation(num_qubits(), num_qubits());
        transformation.foreach_row(
            [](auto& row, const auto row_index) { row[row_index] = 1; });

        std::reverse(gates.begin(), gates.end());
        for (const auto [control, target] : gates) {
            transformation.row(target) ^= transformation.row(control);
        }
        cnot_patel(network_, qubits_, transformation, parameters_.cp_params);
    }

  private:
    constexpr uint32_t num_qubits() const { return qubits_.size(); }

    auto select_row(std::vector<uint32_t> const& selected_columns,
                    dynamic_bitset<uint32_t> const& remaining_rows) {
        assert(remaining_rows.count() > 0);
        auto row_idx = 0u;
        auto max = 0u;
        for (auto i = 0u; i < parity_matrix_.num_rows(); ++i) {
            if (remaining_rows[i] == 0) {
                continue;
            }
            const auto row = parity_matrix_.row(selected_columns, i);
            const auto num_ones = row.count();
            const auto num_zeros = row.size() - num_ones; // Always >= 0
            const auto local_max = std::max(num_ones, num_zeros);
            if (local_max > max) {
                max = local_max;
                row_idx = i;
            }
        }
        return row_idx;
    }

  private:
    Network& network_;
    std::vector<io_id> qubits_;
    parity_terms<uint32_t> parities_;
    matrix_type parity_matrix_;
    std::vector<state_type> state_stack_;
    gray_synth_params parameters_;
};

} // namespace detail

/*! \brief Gray synthesis for {CNOT, Rz} networks.
 *
 * This is the in-place variant of ``gray_synth``, in which the network is
 * passed as a parameter and can potentially already contain some gates. The
 * parameter ``qubits`` provides a qubit mapping to the existing qubits in the
 * network.
 *
 * \param network  A quantum network
 * \param qubits   The subset of qubits the linear reversible circuit acts upon
 * \param parities List of parities and rotation angles to synthesize
 * \param params   The parameters that configure the synthesis process.
 *                 See `gray_synth_params` for details.
 */
template <class Network>
void gray_synth(Network& network, std::vector<io_id> const& qubits,
                parity_terms<uint32_t> const& parities,
                gray_synth_params params = {}) {
    assert(qubits.size() <= 32u);
    if (parities.num_terms() == 0u) {
        return;
    }
    detail::gray_synth_ftor synthesizer(network, qubits, parities, params);
    synthesizer.synthesize();
}

/*! \brief Gray synthesis for {CNOT, Rz} networks.
 *
   \verbatim embed:rst

   This algorithm is based on the work in :cite:`AAM17`.

   \endverbatim
 *
 * \param num_qubits Number of qubits
 * \param parities   List of parities and rotation angles to synthesize
 * \param params     The parameters that configure the synthesis process.
 *                   See `gray_synth_params` for details.
 *
 * \return {CNOT, Rz} network
 *
 * \algtype synthesis
 * \algexpects List of parities and rotation angles to synthesize
 * \algreturns {CNOT, Rz} network
 */
template <class Network>
Network gray_synth(uint32_t num_qubits, parity_terms<uint32_t> const& parities,
                   gray_synth_params params = {}) {
    assert(num_qubits <= 32);
    Network network;
    for (auto i = 0u; i < num_qubits; ++i) {
        network.add_qubit();
    }
    gray_synth(network, network.rewire_map(), parities, params);
    return network;
}

} // namespace tweedledum
