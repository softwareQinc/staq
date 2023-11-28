/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../networks/io_id.hpp"
#include "../../utils/parity_terms.hpp"
#include "gray_synth.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace tweedledum {

namespace detail {

inline void fast_hadamard_transform(std::vector<double>& s) {
    unsigned k{};
    double t{};

    for (auto m = 1u; m < s.size(); m <<= 1u) {
        for (auto i = 0u; i < s.size(); i += (m << 1u)) {
            for (auto j = i, p = k = i + m; j < p; ++j, ++k) {
                t = s[j];
                s[j] += s[k];
                s[k] = t - s[k];
            }
        }
    }
}

} // namespace detail

/*! \brief Gray synthesis for {CNOT, Rz} networks.
 *
 * This is the in-place variant of ``diagonal_synth``, in which the network is
 * passed as a parameter and can potentially already contain some gates.
 * The parameter ``qubits`` provides a qubit mapping to the existing qubits in
 * the network.
 *
 * \param circ   A quantum network
 * \param qubits The subset of qubits the linear reversible circuit acts upon
 * \param angles Angles for diagonal matrix elements
 */
template <class Circuit>
void diagonal_synth(Circuit& circ, std::vector<io_id> const& qubits,
                    std::vector<double> const& angles) {
    /* there are 2^n - 1 angles */
    assert(1u << qubits.size() == angles.size() + 1u);

    /* fast Hadamard transform on extended vector */
    std::vector<double> s(angles.size() + 1, 0.0);
    std::copy(angles.begin(), angles.end(), s.begin() + 1);
    detail::fast_hadamard_transform(s);

    const double factor = 1 << (qubits.size() - 1);
    parity_terms<uint32_t> parities;
    for (uint32_t i = 1u; i < s.size(); ++i) {
        if (s[i] == 0.0)
            continue;
        parities.add_term(i, s[i] / factor);
    }

    gray_synth(circ, qubits, parities);
}

/*! \brief Synthesis of diagonal unitary matrices.
 *
   \verbatim embed:rst

   This algorithm is based on the work in :cite:`SS03`.  It takes as input
   :math:`2^n - 1` real angles :math:`\theta_i` for :math:`i = 1, \dots,
 2^{n-1}` and returns a quantum circuit that realizes the :math:`2^n \times 2^n`
 unitary operation

   :math:`U = \mathrm{diag}(1, e^{-\mathrm{i}\theta_1}, e^{-\mathrm{i}\theta_2},
 \dots, e^{-\mathrm{i}\theta_{2^{n-1}}}).`

   It uses a fast Hadamard transformation to compute angles for parity terms
   that are passed to the Gray synthesis algorithm ``gray_synth``.

   \endverbatim
 *
 * \param angles Angles for diagonal matrix elements
 *
 * \return {CNOT, Rz} network
 *
 * \algtype synthesis
 * \algexpects List of angles in diagonal unitary matrix
 * \algreturns {CNOT, Rz} network
 */
template <class Circuit>
Circuit diagonal_synth(std::vector<double> const& angles) {
    const auto num_qubits = static_cast<uint32_t>(std::log2(angles.size() + 1));
    assert(1u << num_qubits == angles.size() + 1u);
    assert(num_qubits <= 32);
    Circuit circ;
    for (auto i = 0u; i < num_qubits; ++i) {
        circ.add_qubit();
    }
    diagonal_synth(circ, circ.rewire_map(), angles);
    return circ;
}

} // namespace tweedledum
