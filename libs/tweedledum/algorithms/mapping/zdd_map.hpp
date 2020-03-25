/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Kate Smith
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../gates/io3_gate.hpp"
#include "../../io/quil.hpp"
#include "../../io/write_unicode.hpp"
#include "../../networks/netlist.hpp"
#include "../../utils/dd/zdd.hpp"
#include "../../utils/device.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/depth_view.hpp"
#include "../../views/pathsum_view.hpp"
#include "../../views/mapping_view.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tweedledum {

struct zdd_map_params {
    bool verbose = false;
    bool werbose = false;
};

struct zdd_map_stats {
    uint32_t map_coverage = 0u;
    uint32_t num_swaps = 0u;
    // Times
    double time_swap_layers_build = 0.0;
    double time_map_search = 0.0;
    double time_new_network = 0;
    double time_total = 0.0;
};

namespace detail {

template <typename Network>
class find_maximal_partitions_impl {
  public:
    find_maximal_partitions_impl(Network const& network, device const& arch,
                                 zdd_map_params const& ps, zdd_map_stats& st)
        : network_(network), arch_(arch), params_(ps), stats_(st),
          zdd_(network.num_qubits() * arch.num_vertices, 21),
          from_(network.num_qubits()), to_(arch.num_vertices),
          edge_perm_(arch.num_vertices, 0),
          id_virtual_map_(network.num_io(), io_invalid) {
        std::iota(edge_perm_.begin(), edge_perm_.end(), 0);
        uint32_t i = 0;
        network.foreach_io([&](io_id id) {
            if (id.is_qubit()) {
                id_virtual_map_.at(id.index()) = io_id(i++, true);
                phy_id_map_.push_back(id);
            }
        });
    }

    mapping_view<Network> run() {

        init_from();
        init_to();
        init_valid();
        zdd_.garbage_collect();
        init_bad();
        zdd_.garbage_collect();

        // first map always starts at zero-index two qubit gate
        map_start_.push_back(0);

        // Build ZDD that represents all swaps that can be done in parallel
        stopwatch<>::duration_type tm(0);
        call_with_stopwatch(tm, [&]() { return init_swap_layers(); });
        stats_.time_swap_layers_build = to_seconds(tm);

        tm = tm.zero();
        call_with_stopwatch(tm, [&]() { return search_mappings(); });
        stats_.time_map_search = to_seconds(tm);

        // Find the map that covers most double-qubit gates
        uint32_t max_coverage_idx =
            std::max_element(map_coverage_.begin(), map_coverage_.end()) -
            map_coverage_.begin();
        uint32_t partition_start = 0;
        for (uint32_t i = 0; i < index_of_swap.size(); i++) {
            if (index_of_swap[i] >= map_start_[max_coverage_idx]) {
                partition_start = i;
                break;
            }
        }
        // Retrieve all sets from largest partition to pick one for new map
        zdd_.sets_to_vector(maps_[max_coverage_idx], global_found_sets);

        // This below chooses the set to map new circuit to!
        auto current_mapping = choose_mapping();

        // Make new circuit here
        tm = tm.zero();
        stopwatch timer(tm);
        mapping_view mapped_ntk(network_, arch_, true);
        // return mapped_ntk;
        mapped_ntk.set_virtual_phy_map(current_mapping);
        uint32_t count_2q = 0;
        uint32_t index_counter =
            partition_start; // Start at first 2q gate in partition
        network_.foreach_gate([&](auto const& node) {
            auto const& gate = node.gate;
            if (!gate.is_double_qubit()) {
                mapped_ntk.add_gate(gate, gate.target());
                return;
            }
            if (index_of_swap.empty()) {
                // No swaps needed for circuit b/c zero items in index_of_swap
                mapped_ntk.add_gate(gate, gate.control(), gate.target());
                return;
            }
            // If 2q gate is one that needs swap, and its in the correct
            // partition range, add it!
            if (count_2q == index_of_swap[index_counter] &&
                (count_2q < (map_start_[max_coverage_idx] +
                             map_coverage_[max_coverage_idx]))) {
                // Sometimes multiple swaps from layers are needed...
                while (count_2q == index_of_swap[index_counter]) {
                    // Insert as many swaps that are needed in a particular spot
                    mapped_ntk.add_swap(swaps_[index_counter].first,
                                        swaps_[index_counter].second);
                    index_counter++;
                }
                // Insert gate
                mapped_ntk.add_gate(gate, gate.control(), gate.target());
            } else {
                // Insert gate with fixed qubits
                mapped_ntk.add_gate(gate, gate.control(), gate.target());
            }
            count_2q++;
        });
        timer.stop();

        // Stats
        stats_.map_coverage = map_coverage_[max_coverage_idx];
        stats_.num_swaps = swaps_.size();
        stats_.time_new_network = to_seconds(tm);
        stats_.time_total = stats_.time_swap_layers_build +
                            stats_.time_map_search + stats_.time_new_network;
        return mapped_ntk;
    }

  private:
    auto index(uint32_t v, uint32_t p) const {
        return v * arch_.num_vertices + p;
    }

    void init_from() {
        for (auto v = 0u; v < network_.num_qubits(); ++v) {
            auto set = zdd_.bot();
            for (int p = arch_.num_vertices - 1; p >= 0; --p) {
                set = zdd_.union_(set, zdd_.elementary(index(v, p)));
            }
            from_[v] = set;
            zdd_.ref(set);
        }
    }

    void init_to() {
        for (auto p = 0u; p < arch_.num_vertices; ++p) {
            auto set = zdd_.bot();
            for (int v = network_.num_qubits() - 1; v >= 0; --v) {
                set = zdd_.union_(set, zdd_.elementary(index(v, p)));
            }
            to_[p] = set;
            zdd_.ref(set);
        }
    }

    void init_valid() {
        valid_ = zdd_.bot();
        for (auto const& [p, q] : arch_.edges) {
            valid_ = zdd_.union_(
                valid_, zdd_.join(to_[edge_perm_[p]], to_[edge_perm_[q]]));
        }
        zdd_.ref(valid_);
    }

    void init_bad() {
        bad_ = zdd_.bot();
        for (int v = network_.num_qubits() - 1; v >= 0; --v) {
            bad_ = zdd_.union_(bad_, zdd_.choose(from_[v], 2));
        }
        for (int p = arch_.num_vertices - 1; p >= 0; --p) {
            bad_ = zdd_.union_(bad_, zdd_.choose(to_[p], 2));
        }
        zdd_.ref(bad_);
    }

    zdd_base::node map(uint32_t c, uint32_t t) {
        return zdd_.intersection(zdd_.join(from_[c], from_[t]), valid_);
    }

    std::vector<uint32_t> choose_mapping() const {
        uint32_t set_to_use = 0;
        std::vector<uint32_t> used_phys_qubits;
        // Index is pseudo, what is stored is the mapping
        std::vector<int> chosen_mapping(network_.num_qubits(), -1);
        for (auto const& item : global_found_sets[set_to_use]) {
            uint32_t pseudo_qubit = item / network_.num_qubits();
            uint32_t physical_qubit = item % network_.num_qubits();
            chosen_mapping[pseudo_qubit] = physical_qubit;
            used_phys_qubits.push_back(physical_qubit);
        }

        std::vector<uint32_t> unused_phys_qubits;
        for (auto i = 0u; i < chosen_mapping.size(); i++) {
            if (std::find(used_phys_qubits.begin(), used_phys_qubits.end(),
                          i) == used_phys_qubits.end()) {
                unused_phys_qubits.push_back(i);
            }
        }

        uint32_t qubit_ctr = 0;
        for (auto i = 0u; i < chosen_mapping.size(); i++) {
            if (chosen_mapping[i] == -1) {
                chosen_mapping[i] = unused_phys_qubits[qubit_ctr];
                qubit_ctr++;
            }
        }

        // Convert vector<int> to vector<uint32_t>
        std::vector<uint32_t> result;
        for (auto e : chosen_mapping) {
            result.push_back(e);
        }
        return result;
    }

    void search_mappings() {
        uint32_t depth_weight = 0;
        uint32_t map_weight = 1;
        double swap_weight = 1;
        auto m = zdd_.bot();
        // Counts double-qubit gates
        uint32_t count_2q = 0;

        depth_view depth_nkt(network_);
        // Below is where we look for maps!
        depth_nkt.foreach_gate([&](auto const& n, auto node_index) {
            if (!n.gate.is_double_qubit()) {
                return;
            }
            uint32_t c = id_virtual_map_.at(n.gate.control());
            uint32_t t = id_virtual_map_.at(n.gate.target());
            if (m == zdd_.bot()) {
                /* first gate */
                m = map(c, t);
                zdd_.ref(m);
                ++count_2q;
                return;
            }
            auto m_next = map(c, t);
            auto mp = zdd_.nonsupersets(zdd_.join(m, m_next), bad_);
            if (mp != zdd_.bot()) {
                zdd_.deref(m);
                zdd_.ref(mp);
                m = mp;
                ++count_2q;
                return;
            }

            std::vector<uint32_t> new_mappings_cnt(swap_layers_.size(), 0);
            std::vector<uint32_t> depth_count(swap_layers_.size(), 0);
            std::vector<uint32_t> swap_count(swap_layers_.size(), 0);

            for (uint32_t i = 0; i < swap_layers_.size(); i++) {
                swap_count[i] = swap_layers_[i].size();
                // Swap physical qubits
                for (auto const& item : swap_layers_[i]) {
                    std::swap(edge_perm_[arch_.edges[item].first],
                              edge_perm_[arch_.edges[item].second]);
                }
                zdd_.deref(valid_);
                zdd_.garbage_collect();
                init_valid();
                m_next = map(c, t);
                mp = zdd_.nonsupersets(zdd_.join(m, m_next), bad_);
                if (mp == zdd_.bot()) {
                    // Cannot extend map
                    new_mappings_cnt[i] = 0;
                    depth_count[i] = 0;
                } else {
                    // Can extend map.
                    // Determine depth and number of maps_
                    // Move on to next possible swap
                    auto m_prime = m;
                    network_.foreach_gate(
                        [&](auto const& nn) {
                            if (!nn.gate.is_double_qubit()) {
                                return true;
                            }
                            uint32_t cc = id_virtual_map_.at(nn.gate.control());
                            uint32_t tt = id_virtual_map_.at(nn.gate.target());
                            auto m_next_prime = map(cc, tt);
                            auto mp_prime = zdd_.nonsupersets(
                                zdd_.join(m_prime, m_next_prime), bad_);
                            if (mp_prime == zdd_.bot()) {
                                depth_count[i] =
                                    depth_nkt.level(nn) - depth_nkt.level(n);
                                return false;
                            }
                            m_prime = mp_prime;
                            return true;
                        },
                        node_index);
                    new_mappings_cnt[i] = zdd_.count_sets(m_prime);
                    // new_mappings_cnt[i] = zdd_.count_sets(mp);
                    // fmt::print("{}, {}\n", zdd_.count_sets(mp),
                    // zdd_.count_sets(m_prime));
                }
                // Unswap physical qubits for next iteration
                for (auto const& item : swap_layers_[i]) {
                    std::swap(edge_perm_[arch_.edges[item].first],
                              edge_perm_[arch_.edges[item].second]);
                }
                zdd_.deref(valid_);
                zdd_.garbage_collect();
                init_valid();
            }

            // Use below to set score for layers
            std::vector<double> scores(swap_layers_.size(), 0);
            for (uint32_t index = 0; index < depth_count.size(); index++) {
                double inv_swap_cnt;
                if (swap_count[index] == 0) {
                    inv_swap_cnt = 0;
                } else {
                    inv_swap_cnt = 1.0 / swap_count[index];
                }
                scores[index] = (depth_count[index] * depth_weight +
                                 new_mappings_cnt[index] * map_weight) *
                                ((inv_swap_cnt) *swap_weight);
            }
            uint32_t max_index =
                std::max_element(scores.begin(), scores.end()) - scores.begin();
            int max_score = scores[max_index];

            if (max_score == 0) {
                zdd_.ref(m);
                maps_.push_back(m);
                map_start_.push_back(count_2q);
                std::iota(edge_perm_.begin(), edge_perm_.end(), 0u);
                zdd_.deref(valid_);
                zdd_.garbage_collect();
                init_valid();
                m = map(c, t);
                zdd_.ref(m);
            } else {
                for (auto const& item : swap_layers_[max_index]) {
                    std::swap(edge_perm_[arch_.edges[item].first],
                              edge_perm_[arch_.edges[item].second]);
                    swaps_.emplace_back(arch_.edges[item]);
                    index_of_swap.push_back(count_2q);
                }
                zdd_.deref(valid_);
                zdd_.garbage_collect();
                init_valid();

                auto m_next = map(c, t);
                mp = zdd_.nonsupersets(zdd_.join(m, m_next), bad_);
                zdd_.deref(m);
                zdd_.ref(mp);
                m = mp;
            }
            ++count_2q;
        });
        zdd_.ref(m);
        maps_.push_back(m);
        map_start_.push_back(count_2q);

        for (uint32_t i = 0; i < map_start_.size(); i++) {
            if (i == map_start_.size() - 1) {
                map_coverage_.push_back(count_2q - map_start_[i]);
            } else {
                map_coverage_.push_back(map_start_[i + 1] - map_start_[i]);
            }
        }
    }

    void init_swap_layers() {
        zdd_base zdd_swap_layers(arch_.edges.size());
        zdd_swap_layers.build_tautologies();

        auto univ_fam = zdd_swap_layers.tautology();
        std::vector<zdd_base::node> edges_p;

        std::vector<std::vector<uint8_t>> incidents(arch_.num_vertices);
        for (auto i = 0u; i < arch_.edges.size(); ++i) {
            incidents[arch_.edges[i].first].push_back(i);
            incidents[arch_.edges[i].second].push_back(i);
        }

        for (auto& i : incidents) {
            std::sort(i.rbegin(), i.rend());
            auto set = zdd_swap_layers.bot();
            for (auto o : i) {
                set =
                    zdd_swap_layers.union_(set, zdd_swap_layers.elementary(o));
            }
            edges_p.push_back(set);
        }

        auto edges_union = zdd_swap_layers.bot();
        for (int v = network_.num_qubits() - 1; v >= 0; --v) {
            edges_union = zdd_swap_layers.union_(
                edges_union, zdd_swap_layers.choose(edges_p[v], 2));
        }
        auto layers = zdd_swap_layers.nonsupersets(univ_fam, edges_union);
        zdd_swap_layers.sets_to_vector(layers, swap_layers_);
    }

  private:
    using swap_type = std::pair<uint32_t, uint32_t>;

    Network const& network_;
    device const& arch_;
    zdd_map_params const& params_;
    zdd_map_stats& stats_;

    zdd_base zdd_;
    std::vector<zdd_base::node> from_;
    std::vector<zdd_base::node> to_;
    zdd_base::node valid_;
    zdd_base::node bad_;
    std::vector<uint32_t> edge_perm_;

    // vectors that hold sets found in mapping and swap layer zdds,respectively
    std::vector<std::vector<uint32_t>> global_found_sets;
    std::vector<std::vector<uint32_t>> swap_layers_;

    std::vector<zdd_base::node> maps_;

    // vector that holds gate number that swap is needed for
    std::vector<uint32_t> index_of_swap;
    // vector that holds the qubits implemented swaps
    std::vector<swap_type> swaps_;
    // vector that holds index of new maps
    std::vector<uint32_t> map_start_;
    // holds the size of each map
    std::vector<uint32_t> map_coverage_;

    // Qubit normalization
    std::vector<io_id> id_virtual_map_;
    std::vector<io_id> phy_id_map_;
};

} // namespace detail

/*! \brief
 *
 * **Required gate functions:**
 *
 * **Required network functions:**
 *
 */
template <typename Network>
mapping_view<Network> zdd_map(Network& network, device const& arch,
                              zdd_map_params const& ps = {},
                              zdd_map_stats* pst = nullptr) {
    zdd_map_stats st;
    detail::find_maximal_partitions_impl<Network> impl(network, arch, ps, st);
    mapping_view<Network> mapped_network = impl.run();
    if (pst) {
        *pst = st;
    }
    return mapped_network;
}

} // namespace tweedledum
