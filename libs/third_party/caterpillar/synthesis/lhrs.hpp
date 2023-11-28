/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken
| Author(s): Giulia Meuli
*-----------------------------------------------------------------------------*/
#pragma once
#include "../stg_gate.hpp"
#include "strategies/mapping_strategy.hpp"

#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <mockturtle/algorithms/cut_enumeration/spectr_cut.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <tweedledum/algorithms/synthesis/stg.hpp>
#include <stack>

#include <variant>
#include <vector>

using Qubit = tweedledum::io_id;
using SetQubits = std::vector<Qubit>;

namespace caterpillar {

namespace mt = mockturtle;

struct logic_network_synthesis_params {
    /*! \brief Be verbose. */
    bool verbose{false};
};

struct logic_network_synthesis_stats {
    /*! \brief Total runtime. */
    mockturtle::stopwatch<>::duration time_total{0};

    /*! \brief Required number of ancilla. */
    uint32_t required_ancillae{0u};

    /*! \brief output qubits. */
    std::vector<uint32_t> o_indexes;

    /*! \brief input qubits. */
    std::vector<uint32_t> i_indexes;

    void report() const {
        std::cout << fmt::format("[i] total time = {:>5.2f} secs\n",
                                 mockturtle::to_seconds(time_total));
    }
};

namespace detail {

template <class QuantumNetwork, class LogicNetwork,
          class SingleTargetGateSynthesisFn>
class logic_network_synthesis_impl {
  public:
    logic_network_synthesis_impl(QuantumNetwork& qnet, LogicNetwork const& ntk,
                                 mapping_strategy<LogicNetwork>& strategy,
                                 SingleTargetGateSynthesisFn const& stg_fn,
                                 logic_network_synthesis_params const& ps,
                                 logic_network_synthesis_stats& st)
        : qnet(qnet), ntk(ntk), strategy(strategy), stg_fn(stg_fn), ps(ps),
          st(st), node_to_qubit(ntk) {}

    bool run() {
        mockturtle::stopwatch t(st.time_total);
        prepare_inputs();
        prepare_constant(false);
        if (ntk.get_node(ntk.get_constant(false)) !=
            ntk.get_node(ntk.get_constant(true)))
            prepare_constant(true);

        if (const auto result = strategy.compute_steps(ntk); !result) {
            return false;
        }
        strategy.foreach_step([&](auto node, auto action) {
            std::visit(
                overloaded{
                    [](auto) {},
                    [&](compute_action const& action) {
                        const auto t = node_to_qubit[node] = request_ancilla();
                        if (ps.verbose) {
                            std::cout << "[i] compute "
                                      << ntk.node_to_index(node) << " in qubit "
                                      << t;
                            if (action.leaves) {
                                std::cout << " with leaves: ";
                                for (auto l : *action.leaves) {
                                    std::cout << l << " ";
                                }
                            }
                            std::cout << "\n";
                        }
                        if (action.cell_override) {
                            const auto [func, leaves] = *action.cell_override;
                            compute_node_as_cell(node, t, func, leaves);
                        } else if (action.leaves) {
                            compute_big_xor(t, *action.leaves);
                        } else {
                            compute_node(node, t);
                        }
                    },
                    [&](uncompute_action const& action) {
                        const auto t = node_to_qubit[node];
                        if (ps.verbose) {
                            std::cout << "[i] uncompute "
                                      << ntk.node_to_index(node)
                                      << " from qubit " << t;
                            if (action.leaves) {
                                std::cout << " with leaves: ";
                                for (auto l : *action.leaves) {
                                    std::cout << l << " ";
                                }
                            }
                            std::cout << "\n";
                        }
                        if (action.cell_override) {
                            const auto [func, leaves] = *action.cell_override;
                            compute_node_as_cell(node, t, func, leaves);
                        } else if (action.leaves) {
                            compute_big_xor(t, *action.leaves);
                        } else {
                            compute_node(node, t);
                        }
                        release_ancilla(t);
                    },
                    [&](compute_inplace_action const& action) {
                        if (ps.verbose) {
                            std::cout << "[i] compute "
                                      << ntk.node_to_index(node)
                                      << " inplace onto " << action.target_index
                                      << " in qubit "
                                      << node_to_qubit[ntk.index_to_node(
                                             action.target_index)];
                            if (action.leaves) {
                                std::cout << " with leaves: ";
                                for (auto l : *action.leaves) {
                                    std::cout << l << " ";
                                }
                            }
                            std::cout << "\n";
                        }
                        const auto t = node_to_qubit[node] =
                            node_to_qubit[ntk.index_to_node(
                                action.target_index)];
                        if (action.leaves) {
                            compute_big_xor(t, *action.leaves);
                        } else {
                            compute_node_inplace(node, t);
                        }
                    },
                    [&](uncompute_inplace_action const& action) {
                        if (ps.verbose) {
                            std::cout << "[i] uncompute "
                                      << ntk.node_to_index(node)
                                      << " inplace onto " << action.target_index
                                      << " from qubit "
                                      << node_to_qubit[ntk.index_to_node(
                                             action.target_index)];
                            if (action.leaves) {
                                std::cout << " with leaves: ";
                                for (auto l : *action.leaves) {
                                    std::cout << l << " ";
                                }
                            }
                        }
                        const auto t = node_to_qubit[node];
                        if (action.leaves) {
                            compute_big_xor(t, *action.leaves);
                        } else {
                            compute_node_inplace(node, t);
                        }
                    }},
                action);
        });

        prepare_outputs();

        return true;
    }

  private:
    void prepare_inputs() {
        /* prepare primary inputs of logic network */
        ntk.foreach_pi([&](auto n) {
            node_to_qubit[n] = qnet.num_qubits();
            st.i_indexes.push_back(node_to_qubit[n]);
            qnet.add_qubit();
        });
    }

    void prepare_constant(bool value) {
        const auto f = ntk.get_constant(value);
        const auto n = ntk.get_node(f);
        if (ntk.fanout_size(n) == 0)
            return;
        const auto v = ntk.constant_value(n) ^ ntk.is_complemented(f);
        node_to_qubit[n] = qnet.num_qubits();
        qnet.add_qubit();
        if (v)
            qnet.add_gate(tweedledum::gate::pauli_x,
                          tweedledum::io_id(node_to_qubit[n], true));
    }

    uint32_t request_ancilla() {
        if (free_ancillae.empty()) {
            const auto r = qnet.num_qubits();
            st.required_ancillae++;
            qnet.add_qubit();
            return r;
        } else {
            const auto r = free_ancillae.top();
            free_ancillae.pop();
            return r;
        }
    }

    void prepare_outputs() {
        std::unordered_map<mt::node<LogicNetwork>, mt::signal<LogicNetwork>>
            node_to_signals;
        ntk.foreach_po([&](auto s) {
            auto node = ntk.get_node(s);

            if (const auto it = node_to_signals.find(node);
                it != node_to_signals.end()) // node previously referred
            {
                auto new_i = request_ancilla();

                qnet.add_gate(tweedledum::gate::cx,
                              tweedledum::io_id(
                                  node_to_qubit[ntk.node_to_index(node)], true),
                              tweedledum::io_id(new_i, true));
                if (ntk.is_complemented(s) !=
                    ntk.is_complemented(node_to_signals[node])) {
                    qnet.add_gate(tweedledum::gate::pauli_x,
                                  tweedledum::io_id(new_i, true));
                }
                st.o_indexes.push_back(new_i);
            } else // node never referred
            {
                if (ntk.is_complemented(s)) {
                    qnet.add_gate(
                        tweedledum::gate::pauli_x,
                        tweedledum::io_id(
                            node_to_qubit[ntk.node_to_index(node)], true));
                }
                node_to_signals[node] = s;
                st.o_indexes.push_back(node_to_qubit[ntk.node_to_index(node)]);
            }
        });
    }

    void release_ancilla(uint32_t q) { free_ancillae.push(q); }

    template <int Fanin>
    std::array<uint32_t, Fanin>
    get_fanin_as_literals(mt::node<LogicNetwork> const& n) {
        std::array<uint32_t, Fanin> controls;
        ntk.foreach_fanin(n, [&](auto const& f, auto i) {
            controls[i] = (ntk.node_to_index(ntk.get_node(f)) << 1) |
                          ntk.is_complemented(f);
        });
        return controls;
    }

    SetQubits get_fanin_as_qubits(mt::node<LogicNetwork> const& n) {
        SetQubits controls;
        ntk.foreach_fanin(n, [&](auto const& f) {
            assert(!ntk.is_complemented(f));
            controls.push_back(tweedledum::io_id(
                node_to_qubit[ntk.node_to_index(ntk.get_node(f))], true));
        });
        return controls;
    }

    void compute_big_xor(uint32_t t, std::vector<uint32_t> leaves) {
        for (auto control : leaves) {
            if (node_to_qubit[control] != t) {
                qnet.add_gate(tweedledum::gate::cx,
                              tweedledum::io_id(node_to_qubit[control], true),
                              tweedledum::io_id(t, true));
            }
        }
    }

    void compute_node(mt::node<LogicNetwork> const& node, uint32_t t) {
        if constexpr (mt::has_is_and_v<LogicNetwork>) {
            if (ntk.is_and(node)) {
                auto controls = get_fanin_as_literals<2>(node);

                SetQubits pol_controls;
                pol_controls.emplace_back(
                    node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                    controls[0] & 1);
                pol_controls.emplace_back(
                    node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                    controls[1] & 1);

                compute_and(pol_controls, t);
                return;
            }
        }
        if constexpr (mt::has_is_or_v<LogicNetwork>) {
            if (ntk.is_or(node)) {
                auto controls = get_fanin_as_literals<2>(node);

                SetQubits pol_controls;
                pol_controls.emplace_back(
                    node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                    !(controls[0] & 1));
                pol_controls.emplace_back(
                    node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                    !(controls[1] & 1));

                compute_or(pol_controls, t);
                return;
            }
        }
        if constexpr (mt::has_is_xor_v<LogicNetwork>) {
            if (ntk.is_xor(node)) {

                auto controls = get_fanin_as_literals<2>(node);
                compute_xor(node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                            node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                            (controls[0] & 1) != (controls[1] & 1), t);
                return;
            }
        }
        if constexpr (mt::has_is_xor3_v<LogicNetwork>) {
            if (ntk.is_xor3(node)) {
                auto controls = get_fanin_as_literals<3>(node);

                /* Is XOR3 in fact an XOR2? */
                if (ntk.is_constant(ntk.index_to_node(controls[0] >> 1))) {
                    compute_xor(
                        node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                        ((controls[0] & 1) != (controls[1] & 1)) !=
                            (controls[2] & 1),
                        t);
                } else {
                    compute_xor3(
                        node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                        ((controls[0] & 1) != (controls[1] & 1)) !=
                            (controls[2] & 1),
                        t);
                }
                return;
            }
        }
        if constexpr (mt::has_is_maj_v<LogicNetwork>) {
            if (ntk.is_maj(node)) {
                auto controls = get_fanin_as_literals<3>(node);
                /* Is XOR3 in fact an AND or OR? */
                if (ntk.is_constant(ntk.index_to_node(controls[0] >> 1))) {
                    if (controls[0] & 1) {
                        SetQubits pol_controls;
                        pol_controls.emplace_back(
                            node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                            !(controls[1] & 1));
                        pol_controls.emplace_back(
                            node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                            !(controls[2] & 1));

                        compute_or(pol_controls, tweedledum::io_id(t, true));
                    } else {
                        SetQubits pol_controls;
                        pol_controls.emplace_back(
                            node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                            controls[1] & 1);
                        pol_controls.emplace_back(
                            node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                            controls[2] & 1);

                        compute_and(pol_controls, tweedledum::io_id(t, true));
                    }
                } else {
                    compute_maj(
                        node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                        controls[0] & 1, controls[1] & 1, controls[2] & 1, t);
                }
                return;
            }
        }
        if constexpr (mt::has_node_function_v<LogicNetwork>) {
            kitty::dynamic_truth_table tt = ntk.node_function(node);
            auto clone = tt.construct();
            kitty::create_parity(clone);

            if (tt == clone) {
                const auto controls = get_fanin_as_qubits(node);
                compute_xor_block(controls, tweedledum::io_id(t, true));
            } else {
                // In this case, the procedure works a bit different and
                // retrieves the controls directly as mapped qubits.  We assume
                // that the inputs cannot be complemented, e.g., in the case of
                // k-LUT networks.
                const auto controls = get_fanin_as_qubits(node);
                compute_lut(ntk.node_function(node), controls,
                            tweedledum::io_id(t, true));
            }
        }
    }

    void compute_node_as_cell(mt::node<LogicNetwork> const& node, uint32_t t,
                              kitty::dynamic_truth_table const& func,
                              std::vector<uint32_t> const& leave_indexes) {
        (void) node;

        /* get control qubits */
        SetQubits controls;
        for (auto l : leave_indexes) {
            controls.push_back(
                tweedledum::io_id(node_to_qubit[ntk.node_to_index(l)], true));
        }

        compute_lut(func, controls, tweedledum::io_id(t, true));
    }

    void compute_node_inplace(mt::node<LogicNetwork> const& node, uint32_t t) {
        if constexpr (mt::has_is_xor_v<LogicNetwork>) {
            if (ntk.is_xor(node)) {
                auto controls = get_fanin_as_literals<2>(node);
                compute_xor_inplace(
                    node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                    node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                    (controls[0] & 1) != (controls[1] & 1), t);
                return;
            }
        }
        if constexpr (mt::has_is_xor3_v<LogicNetwork>) {
            if (ntk.is_xor3(node)) {
                auto controls = get_fanin_as_literals<3>(node);

                /* Is XOR3 in fact an XOR2? */
                if (ntk.is_constant(ntk.index_to_node(controls[0] >> 1))) {
                    compute_xor_inplace(
                        node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                        ((controls[0] & 1) != (controls[1] & 1)) !=
                            (controls[2] & 1),
                        t);
                } else {
                    compute_xor3_inplace(
                        node_to_qubit[ntk.index_to_node(controls[0] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[1] >> 1)],
                        node_to_qubit[ntk.index_to_node(controls[2] >> 1)],
                        ((controls[0] & 1) != (controls[1] & 1)) !=
                            (controls[2] & 1),
                        t);
                }
                return;
            }
        }
        if constexpr (mt::has_node_function_v<LogicNetwork>) {
            const auto controls = get_fanin_as_qubits(node);
            compute_xor_block(controls, tweedledum::io_id(t, true));
        }
    }

    void compute_and(SetQubits controls, uint32_t t) {
        qnet.add_gate(tweedledum::gate::mcx, controls, SetQubits{t});
    }

    void compute_or(SetQubits controls, uint32_t t) {
        qnet.add_gate(tweedledum::gate::mcx, controls, SetQubits{t});
        qnet.add_gate(tweedledum::gate::pauli_x, tweedledum::io_id(t, true));
    }

    void compute_xor(uint32_t c1, uint32_t c2, bool inv, uint32_t t) {
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true),
                      tweedledum::io_id(t, true));
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true),
                      tweedledum::io_id(t, true));
        if (inv)
            qnet.add_gate(tweedledum::gate::pauli_x,
                          tweedledum::io_id(t, true));
    }

    void compute_xor3(uint32_t c1, uint32_t c2, uint32_t c3, bool inv,
                      uint32_t t) {
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true),
                      tweedledum::io_id(t, true));
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true),
                      tweedledum::io_id(t, true));
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c3, true),
                      tweedledum::io_id(t, true));
        if (inv)
            qnet.add_gate(tweedledum::gate::pauli_x,
                          tweedledum::io_id(t, true));
    }

    void compute_maj(uint32_t c1, uint32_t c2, uint32_t c3, bool p1, bool p2,
                     bool p3, uint32_t t) {
        if (p1)
            qnet.add_gate(tweedledum::gate::pauli_x, c1);
        if (!p2) /* control 2 behaves opposite */
            qnet.add_gate(tweedledum::gate::pauli_x, c2);
        if (p3)
            qnet.add_gate(tweedledum::gate::pauli_x, c3);

        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true), c2);
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c3, true), c1);
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c3, true), t);

        SetQubits controls;
        controls.push_back(tweedledum::io_id(c1, true));
        controls.push_back(tweedledum::io_id(c2, true));
        qnet.add_gate(tweedledum::gate::mcx, controls, SetQubits{t});

        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c3, true), c1);
        qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true), c2);

        if (p3)
            qnet.add_gate(tweedledum::gate::pauli_x, c3);
        if (!p2)
            qnet.add_gate(tweedledum::gate::pauli_x, c2);
        if (p1)
            qnet.add_gate(tweedledum::gate::pauli_x, c1);
    }

    void compute_xor_block(SetQubits const& controls, Qubit t) {
        for (auto c : controls) {
            if (c != t)
                qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c, true),
                              t);
        }
    }

    void compute_lut(kitty::dynamic_truth_table const& function,
                     SetQubits const& controls, Qubit t) {
        auto qubit_map = controls;
        qubit_map.push_back(t);
        stg_fn(qnet, qubit_map, function);
    }

    void compute_xor_inplace(uint32_t c1, uint32_t c2, bool inv, uint32_t t) {
        if (c1 == t && c2 != t) {
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true),
                          c1);
        } else if (c2 == t && c1 != t) {
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true),
                          c2);
        } else if (c1 != t && c2 != t && c1 != c2) {
            // std::cerr << "[e] target does not match any control in
            // in-place\n";
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true), t);
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true), t);
        }
        if (inv)
            qnet.add_gate(tweedledum::gate::pauli_x, t);
    }

    void compute_xor3_inplace(uint32_t c1, uint32_t c2, uint32_t c3, bool inv,
                              uint32_t t) {
        if (c1 == t) {
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true),
                          tweedledum::io_id(c1, true));
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c3, true),
                          tweedledum::io_id(c1, true));
        } else if (c2 == t) {
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true),
                          c2);
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c3, true),
                          c2);
        } else if (c3 == t) {
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true),
                          c3);
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true),
                          c3);
        } else {
            // std::cerr << "[e] target does not match any control in
            // in-place\n";
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c1, true), t);
            qnet.add_gate(tweedledum::gate::cx, tweedledum::io_id(c2, true), t);
        }
        if (inv)
            qnet.add_gate(tweedledum::gate::pauli_x, t);
    }

  private:
    QuantumNetwork& qnet;
    LogicNetwork const& ntk;
    mapping_strategy<LogicNetwork>& strategy;
    SingleTargetGateSynthesisFn const& stg_fn;
    logic_network_synthesis_params const& ps;
    logic_network_synthesis_stats& st;
    mt::node_map<uint32_t, LogicNetwork> node_to_qubit;
    std::stack<uint32_t> free_ancillae;
}; // namespace detail

} // namespace detail

/*! \brief Hierarchical synthesis based on a logic network
 *
 * This algorithm used hierarchical synthesis and computes a reversible network
 * for each gate in the circuit and computes the intermediate result to an
 * ancilla line.  The node may be computed out-of-place or in-place.  The
 * order in which nodes are computed and uncomputed, and whether they are
 * computed out-of-place or in-place is determined by a separate mapper
 * component `MappingStrategy` that is passed as template parameter to the
 * function.
 */
template <class QuantumNetwork, class LogicNetwork,
          class SingleTargetGateSynthesisFn = tweedledum::stg_from_pprm>
bool logic_network_synthesis(QuantumNetwork& qnet, LogicNetwork const& ntk,
                             mapping_strategy<LogicNetwork>& strategy,
                             SingleTargetGateSynthesisFn const& stg_fn = {},
                             logic_network_synthesis_params const& ps = {},
                             logic_network_synthesis_stats* pst = nullptr) {
    static_assert(mt::is_network_type_v<LogicNetwork>,
                  "LogicNetwork is not a network type");

    logic_network_synthesis_stats st;
    detail::logic_network_synthesis_impl<QuantumNetwork, LogicNetwork,
                                         SingleTargetGateSynthesisFn>
        impl(qnet, ntk, strategy, stg_fn, ps, st);
    const auto result = impl.run();
    if (ps.verbose) {
        st.report();
    }

    if (pst) {
        *pst = st;
    }

    return result;
}

} /* namespace caterpillar */
