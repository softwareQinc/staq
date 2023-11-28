/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken and Giulia Meuli
*-----------------------------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <memory>
#include <stack>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_bench.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/cut_view.hpp>
#include <mockturtle/views/mapping_view.hpp>

#include <fmt/format.h>

#include "eager_mapping_strategy.hpp"
#include "mapping_strategy.hpp"

namespace caterpillar {

struct best_fit_mapping_strategy_params {
    /* cut size for large cuts in pre mapping */
    uint32_t cut_size = 16u;

    /* minimum cut size for remapping */
    uint32_t cut_lower_bound = 4u;
};

namespace detail {

using namespace mockturtle;

// This could eventually be moved to mockturtle, it seems to be useful for more
// cases
template <class Ntk>
class cell_view : public Ntk {
  public:
    cell_view(Ntk const& ntk)
        : Ntk(ntk),
          _cell_fanout(
              std::make_shared<typename decltype(_cell_fanout)::element_type>(
                  ntk, 0u)),
          _node_to_index(
              std::make_shared<typename decltype(_node_to_index)::element_type>(
                  ntk, 0u)),
          _index_to_node(std::make_shared<typename decltype(
                             _index_to_node)::element_type>()) {
        (*_node_to_index)[ntk.get_constant(false)] = 0;
        _index_to_node->reserve(Ntk::num_cells() + Ntk::num_pis() + 2);
        _index_to_node->push_back(ntk.get_node(ntk.get_constant(false)));
        if (ntk.get_node(ntk.get_constant(true)) !=
            ntk.get_node(ntk.get_constant(false))) {
            (*_node_to_index)[ntk.get_constant(true)] = 1;
            _index_to_node->push_back(ntk.get_node(ntk.get_constant(true)));
            _num_constants++;
        }
        Ntk::foreach_pi([&](auto n) {
            (*_node_to_index)[n] = _index_to_node->size();
            _index_to_node->push_back(n);
        });
        init_fanout();
    }

    template <class Fn>
    void foreach_node(Fn&& fn) const {
        Ntk::foreach_node([&](auto n) {
            if (Ntk::is_constant(n) || Ntk::is_pi(n) || Ntk::is_cell_root(n)) {
                fn(n);
            }
        });
    }

    template <class Fn>
    void foreach_fanin(node<Ntk> const& n, Fn&& fn) const {
        Ntk::foreach_cell_fanin(n, [&](auto n2) { fn(Ntk::make_signal(n2)); });
    }

    uint32_t size() const {
        return _num_constants + Ntk::num_pis() + Ntk::num_cells();
    }

    uint32_t num_gates() const { return Ntk::num_cells(); }

    uint32_t node_to_index(node<Ntk> const& n) const {
        return (*_node_to_index)[n];
    }

    node<Ntk> index_to_node(uint32_t index) const {
        return (*_index_to_node)[index];
    }

  private:
    void init_fanout() {
        Ntk::foreach_gate([&](auto n) {
            if (!Ntk::is_cell_root(n))
                return true;

            (*_node_to_index)[n] = _index_to_node->size();
            _index_to_node->push_back(n);
            Ntk::foreach_cell_fanin(n, [&](auto n2) { (*_cell_fanout)[n2]++; });

            return true;
        });
        Ntk::foreach_po([&](auto const& f) { (*_cell_fanout)[f]++; });
    }

  private:
    uint32_t _num_constants{1u};
    std::shared_ptr<node_map<uint32_t, Ntk>> _cell_fanout;
    std::shared_ptr<node_map<uint32_t, Ntk>> _node_to_index;
    std::shared_ptr<std::vector<node<Ntk>>> _index_to_node;
};

} // namespace detail

namespace mt = mockturtle;

template <class LogicNetwork>
class best_fit_mapping_strategy : public mapping_strategy<LogicNetwork> {
  public:
    best_fit_mapping_strategy(best_fit_mapping_strategy_params const& ps = {})
        : ps(ps) {
        static_assert(mt::is_network_type_v<LogicNetwork>,
                      "LogicNetwork is not a network type");
    }

    bool compute_steps(LogicNetwork const& ntk) override {
        _ntk = ntk;
        run();
        return true;
    }

  private:
    void run() {
        /* outer LUT mapping pass is without computing the truth table */
        mt::mapping_view mapped_ntk{_ntk};
        mt::lut_mapping_params lm_ps;
        lm_ps.cut_enumeration_ps.cut_size = ps.cut_size;
        mt::lut_mapping(mapped_ntk, lm_ps);

        detail::cell_view<decltype(mapped_ntk)> cell_ntk{mapped_ntk};

        eager_mapping_strategy<decltype(cell_ntk)> strategy;
        strategy.compute_steps(cell_ntk);
        const auto [total_ancilla, steps] = first_mapping_pass(strategy);

        /* map the cells */
        for (auto const& [n, action, num_dirty_ancilla] : steps) {
            auto num_clean_ancilla = total_ancilla - num_dirty_ancilla;

            std::vector<mt::node<LogicNetwork>> leaves;
            mapped_ntk.foreach_cell_fanin(n,
                                          [&](auto c) { leaves.push_back(c); });
            mt::cut_view cut{_ntk, leaves, n};
            mt::mapping_view<decltype(cut), true> mapped_cut{cut};
            mt::lut_mapping_params lm_ps;
            uint32_t best_cut_size = leaves.size();
            while (best_cut_size > ps.cut_lower_bound) {
                lm_ps.cut_enumeration_ps.cut_size = best_cut_size - 1;
                mt::lut_mapping<decltype(mapped_cut), true>(mapped_cut, lm_ps);
                if (mapped_cut.num_cells() > num_clean_ancilla + 1) {
                    break;
                } else {
                    best_cut_size--;
                }
            }

            if (best_cut_size == leaves.size()) {
                std::vector<uint32_t> leave_indexes;
                for (auto l : leaves) {
                    leave_indexes.push_back(_ntk.node_to_index(l));
                }
                const auto func = mt::simulate<kitty::dynamic_truth_table>(
                    cut, mt::default_simulator<kitty::dynamic_truth_table>(
                             leaves.size()))[0];
                if (std::holds_alternative<compute_action>(action)) {
                    this->steps().emplace_back(
                        n, compute_action{{},
                                          std::make_pair(func, leave_indexes)});
                } else {
                    this->steps().emplace_back(
                        n, uncompute_action{
                               {}, std::make_pair(func, leave_indexes)});
                }
            } else {
                lm_ps.cut_enumeration_ps.cut_size = best_cut_size;
                mt::lut_mapping<decltype(mapped_cut), true>(mapped_cut, lm_ps);

                auto it = this->steps().end();
                mt::node<LogicNetwork> po;
                bool is_computing =
                    std::holds_alternative<compute_action>(action);
                cut.foreach_po([&](auto f) {
                    po = cut.get_node(f);
                    return false;
                });
                mapped_cut.foreach_gate([&](auto cell) {
                    if (!mapped_cut.is_cell_root(cell))
                        return true;
                    std::vector<uint32_t> cell_leaves;
                    mapped_cut.foreach_cell_fanin(cell, [&](auto fanin) {
                        cell_leaves.push_back(_ntk.node_to_index(fanin));
                    });

                    if (cell == po) {
                        if (is_computing) {
                            it = this->steps().emplace(
                                it, cell,
                                compute_action{
                                    {},
                                    std::make_pair(
                                        mapped_cut.cell_function(cell),
                                        cell_leaves)});
                        } else {
                            it = this->steps().emplace(
                                it, cell,
                                uncompute_action{
                                    {},
                                    std::make_pair(
                                        mapped_cut.cell_function(cell),
                                        cell_leaves)});
                        }

                        ++it;
                    } else {
                        it = this->steps().emplace(
                            it, cell,
                            compute_action{
                                {},
                                std::make_pair(mapped_cut.cell_function(cell),
                                               cell_leaves)});
                        ++it;
                        it = this->steps().emplace(
                            it, cell,
                            uncompute_action{
                                {},
                                std::make_pair(mapped_cut.cell_function(cell),
                                               cell_leaves)});
                    }

                    return true;
                });
            }
        }
    }

    template <class MappingStrategy>
    std::pair<uint32_t,
              std::vector<std::tuple<mt::node<LogicNetwork>,
                                     mapping_strategy_action, uint32_t>>>
    first_mapping_pass(MappingStrategy const& strategy) {
        std::vector<std::tuple<mt::node<LogicNetwork>, mapping_strategy_action,
                               uint32_t>>
            first_pass_steps;

        uint32_t next_ancilla = 0u, current_used_ancilla = 0u;
        std::stack<uint32_t> free_list;
        mt::node_map<uint32_t, LogicNetwork> node_to_qubit(_ntk);
        auto const request_ancilla = [&]() {
            if (free_list.empty()) {
                return next_ancilla++;
            } else {
                const auto q = free_list.top();
                free_list.pop();
                return q;
            }
        };

        strategy.foreach_step([&](auto node, auto action) {
            std::visit(
                detail::overloaded{
                    [](auto) {},
                    [&](compute_action const&) {
                        node_to_qubit[node] = request_ancilla();
                        first_pass_steps.emplace_back(node, action,
                                                      ++current_used_ancilla);
                    },
                    [&](uncompute_action const&) {
                        free_list.push(node_to_qubit[node]);
                        first_pass_steps.emplace_back(node, action,
                                                      current_used_ancilla--);
                    },
                    [&](compute_inplace_action const&) { assert(false); },
                    [&](uncompute_inplace_action const&) { assert(false); }},
                action);
        });

        return {next_ancilla, first_pass_steps};
    }

  private:
    /* some parameters that need to be extracted */
    best_fit_mapping_strategy_params ps;

    LogicNetwork _ntk;
};

} // namespace caterpillar
