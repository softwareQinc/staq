/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken
*-----------------------------------------------------------------------------*/
#include <cstdint>
#include <optional>
#include <vector>

#include <mockturtle/traits.hpp>
#include <tweedledum/gates/gate_base.hpp>

namespace caterpillar {

/*! \brief Convert reversible quantum circuit into logic network.
 *
 * This function creates a logic network from a reversible circuit.  If the
 * quantum circuit contains a non-classical gate, it will return `std::nullopt`,
 * otherwise an optional value that contains a logic network.
 *
 * \param circ Reversible quantum circuit
 * \param inputs Qubits which are primary inputs (all other qubits are assumed
 * to be 0) \param outputs Qubits for which a primary output should be created
 */
template <class LogicNetwork, class QuantumCircuit>
std::optional<LogicNetwork>
circuit_to_logic_network(QuantumCircuit const& circ,
                         std::vector<uint32_t> const& inputs,
                         std::vector<uint32_t> const& outputs) {
    using namespace mockturtle;
    using namespace tweedledum;

    static_assert(is_network_type_v<LogicNetwork>,
                  "LogicNetwork is not a network type");
    static_assert(has_create_pi_v<LogicNetwork>,
                  "Ntk does not implement the create_pi method");
    static_assert(has_create_po_v<LogicNetwork>,
                  "Ntk does not implement the create_po method");
    static_assert(has_create_nary_and_v<LogicNetwork>,
                  "Ntk does not implement the create_nary_and method");
    static_assert(has_create_xor_v<LogicNetwork>,
                  "Ntk does not implement the create_xor method");

    LogicNetwork ntk;

    std::vector<signal<LogicNetwork>> qubit_to_signal(circ.num_qubits(),
                                                      ntk.get_constant(false));
    for (auto q : inputs) {
        qubit_to_signal[q] = ntk.create_pi();
    }

    bool error{false};
    circ.foreach_cgate([&](auto n) {
        /* check whether gate is reversible */
        if (!(n.gate.is(gate_set::pauli_x) || n.gate.is(gate_set::cx) ||
              n.gate.is(gate_set::mcx))) {
            error = true;
            return false;
        }

        std::vector<signal<LogicNetwork>> controls;
        n.gate.foreach_control([&](auto c) {
            controls.push_back(qubit_to_signal[c] ^ c.is_complemented());
        });

        const auto ctrl_signal = ntk.create_nary_and(controls);

        n.gate.foreach_target([&](auto t) {
            qubit_to_signal[t] =
                ntk.create_xor(qubit_to_signal[t], ctrl_signal);
        });

        return true;
    });

    if (error) {
        return std::nullopt;
    }

    for (auto q : outputs) {
        ntk.create_po(qubit_to_signal[q]);
    }

    return ntk;
}

} // namespace caterpillar
