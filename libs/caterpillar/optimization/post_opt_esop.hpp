/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "optimization_graph.hpp"

#include <cstdint>
#include <iostream>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/esop.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>
#include <kitty/spectral.hpp>
#include <tweedledum/algorithms/synthesis/gray_synth.hpp>
#include <tweedledum/gates/gate_base.hpp>

#include <vector>

namespace caterpillar {

namespace td = tweedledum;

struct Control {
    uint32_t bit;
    bool polarity = true;
    Control(uint32_t bit, bool polarity) : bit(bit), polarity(polarity) {}
};

template <class Network>
void add_gate_with_neg_contr(Network& net, td::gate_base gate_type,
                             std::vector<Control> controls,
                             std::vector<uint32_t> target) {
    std::vector<uint32_t> control_lines;
    std::vector<uint32_t> negation_targets;

    for (auto c : controls) {
        control_lines.push_back(c.bit);
        if (c.polarity == false)
            negation_targets.push_back(c.bit);
    }

    for (auto n : negation_targets)
        net.add_gate(td::gate::pauli_x, n);

    net.add_gate(gate_type, control_lines, target);

    for (auto n : negation_targets)
        net.add_gate(td::gate::pauli_x, n);
}

template <class Network>
void from_cube_to_toffoli(Network& net, const kitty::cube& cube,
                          const std::vector<uint32_t>& qubit_map) {
    std::vector<Control> controls;
    std::vector<uint32_t> target = {qubit_map.back()};

    auto bits = cube._bits;
    auto mask = cube._mask;

    for (auto v = 0; v < 32; ++v) {
        if (mask & 1)
            controls.push_back((bits & 1) ? Control(qubit_map[v], true)
                                          : Control(qubit_map[v], false));

        bits >>= 1;
        mask >>= 1;
    }

    add_gate_with_neg_contr(net, td::gate::mcx, controls, target);
}

template <class Network>
void add_equivalent_to_net_property1(Network& net, const kitty::cube& a,
                                     const kitty::cube& b,
                                     std::vector<uint32_t> const& qubit_map) {

    auto a_notb = a._mask & (~b._mask);
    auto b_nota = (~a._mask) & b._mask;

    auto one = count_set(a_notb) == 1 ? a_notb : b_nota;
    auto one_cube = count_set(a_notb) == 1 ? a : b;
    auto other = count_set(a_notb) == 1 ? b_nota : a_notb;
    auto other_cube = count_set(a_notb) == 1 ? b : a;

    std::vector<uint32_t> target = {qubit_map[log2(one)]};

    std::vector<Control> controls;
    uint32_t to_create = other;

    for (unsigned i = 0; i < 32; i++) {
        if ((to_create & 1) == 1)
            controls.push_back((!(((other_cube._bits & (1 << i)) >> i) == 1))
                                   ? Control(qubit_map[i], false)
                                   : Control(qubit_map[i], true));

        to_create = to_create >> 1;
    }

    add_gate_with_neg_contr(net, td::gate::mcx, controls, target);

    from_cube_to_toffoli(net, one_cube, qubit_map);

    add_gate_with_neg_contr(net, td::gate::mcx, controls, target);
}

template <class Network>
void add_equivalent_to_net_property2(Network& net, const kitty::cube& a,
                                     const kitty::cube& b,
                                     std::vector<uint32_t> const& qubit_map) {
    auto diff_pol = a._bits ^ b._bits;

    if (count_set(diff_pol) >= 1) {
        bool done = false;
        std::vector<uint32_t> cnot_targets;
        std::vector<uint32_t> cnot_control;
        std::vector<Control> common_controls;
        std::vector<uint32_t> common_target = {qubit_map.back()};

        for (int i = 0; i < 32; ++i) {
            if ((diff_pol >> i) & 1) {
                if (!done) {
                    cnot_control.push_back(qubit_map[i]);
                    done = true;
                    if (((a._bits >> i) & 1) == 1) {
                        for (int j = 0; j < 32; ++j) {
                            if ((((b._mask >> j) & 1) == 1) && (j != i))
                                common_controls.push_back(
                                    b._bits >> j == 1
                                        ? Control(qubit_map[j], true)
                                        : Control(qubit_map[j], false));
                        }
                    } else if (((b._bits >> i) & 1) == 1) {
                        for (int j = 0; j < 32; ++j) {
                            if ((((a._mask >> j) & 1) == 1) && (j != i))
                                common_controls.push_back(
                                    a._bits >> j == 1
                                        ? Control(qubit_map[j], true)
                                        : Control(qubit_map[j], false));
                        }
                    }
                } else
                    cnot_targets.push_back(qubit_map[i]);
            }
        }

        for (auto target : cnot_targets)
            net.add_gate(td::gate::mcx, cnot_control, {target});

        add_gate_with_neg_contr(net, td::gate::mcx, common_controls,
                                common_target);

        for (auto target : cnot_targets)
            net.add_gate(td::gate::mcx, cnot_control, {target});
    }
}

inline optimized_esop match_pairing(std::vector<kitty::cube> esop) {
    auto g = optimization_graph(esop);
    return g.match_properties();
}

/* gets a vect of cubes with the first num_pairs that can be paired according to
the pairing type generate the resulting network of cubes */
template <class Network>
void opt_stg_from_esop(Network& net, optimized_esop o_esop,
                       std::vector<uint32_t> const& qubit_map) {
    /* foreach elem into pairing, put the first two cubes into a function that
      gets the eq circuit and add the gate to the network */
    uint index = 0;
    for (auto type : o_esop.pairing) {
        auto a = o_esop.cubes[index];
        auto b = o_esop.cubes[index + 1];

        if (type == 1)
            add_equivalent_to_net_property1(net, a, b, qubit_map);

        if (type == 2)
            add_equivalent_to_net_property2(net, a, b, qubit_map);

        index = index + 2;
    }
    for (uint i = index; i < o_esop.cubes.size(); i++) {
        from_cube_to_toffoli(net, o_esop.cubes[i], qubit_map);
    }
}

} // namespace caterpillar
