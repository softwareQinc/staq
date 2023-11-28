/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../../gates/gate_base.hpp"
#include "../../../gates/gate_lib.hpp"
#include "../../../networks/io_id.hpp"

#include <array>
#include <vector>

namespace tweedledum::detail {

/*! \brief
 */
template <typename Network>
void cccx(Network& network, std::array<io_id, 4> const& controls,
          std::vector<io_id> const& targets) {
    const auto a = controls[0];
    const auto b = controls[1];
    const auto c = controls[2];
    const auto target = targets[0];

    // Find helper qubit
    auto helper = network.foreach_qubit([&](io_id qid) -> bool {
        if (qid == a || qid == b || qid == c) {
            return true;
        }
        for (auto t : targets) {
            if (qid == t) {
                return true;
            }
        }
        // will return the current qid
        return false;
    });
    assert(helper != io_invalid);

    for (auto i = 1u; i < targets.size(); ++i) {
        network.add_gate(gate::cx, target, targets[i]);
    }

    // R1-TOF(a, b, helper)
    network.add_gate(gate::hadamard, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, b, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::cx, a, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, b, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::hadamard, helper);

    // S-R2-TOF(c, helper, target)
    network.add_gate(gate::hadamard, target);
    network.add_gate(gate::cx, target, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::cx, c, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, target, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::cx, c, helper);
    network.add_gate(gate::t, helper);

    // R1-TOF^-1(a, b, helper)
    network.add_gate(gate::hadamard, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, b, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::cx, a, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, b, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::hadamard, helper);

    // S-R2-TOF^-1(c, helper, target)
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::cx, c, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, target, helper);
    network.add_gate(gate::t_dagger, helper);
    network.add_gate(gate::cx, c, helper);
    network.add_gate(gate::t, helper);
    network.add_gate(gate::cx, target, helper);
    network.add_gate(gate::hadamard, target);

    for (auto i = 1u; i < targets.size(); ++i) {
        network.add_gate(gate::cx, target, targets[i]);
    }
}

} // namespace tweedledum::detail