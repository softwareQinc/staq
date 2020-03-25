/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../optimization/optimization_graph.hpp"
#include "../optimization/post_opt_esop.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/esop.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>
#include <kitty/properties.hpp>
#include <kitty/spectral.hpp>

#include <tweedledum/algorithms/synthesis/gray_synth.hpp>
#include <tweedledum/networks/netlist.hpp>

#include <easy/esop/constructors.hpp>
#include <easy/esop/cost.hpp>

namespace caterpillar {

namespace td = tweedledum;

struct stg_from_esop {
    using esop_synthesis_fn_t = std::function<std::vector<kitty::cube>(
        kitty::dynamic_truth_table const&)>;
    stg_from_esop(esop_synthesis_fn_t esop_synthesis,
                  bool optimize_esop = false)
        : esop_synthesis_(esop_synthesis), optimize_esop_(optimize_esop) {}

    template <class Network>
    void operator()(Network& network,
                    kitty::dynamic_truth_table const& function,
                    std::vector<uint32_t> const& qubit_map) {
        const auto num_controls = function.num_vars();
        assert(qubit_map.size() == static_cast<std::size_t>(num_controls) + 1u);

        std::vector<uint32_t> target = {qubit_map.back()};
        const auto cubes = esop_synthesis_(function);

        optimized_esop opt_esop;
        if (optimize_esop_) {
            opt_esop = match_pairing(cubes);
        } else {
            opt_esop.cubes = cubes;
        }

        opt_stg_from_esop(network, opt_esop, qubit_map);
    }

  private:
    esop_synthesis_fn_t esop_synthesis_;
    bool optimize_esop_{false};
};

struct stg_from_exact_synthesis {
  public:
    explicit stg_from_exact_synthesis(
        std::function<int(kitty::cube)> const& cost_fn =
            [](kitty::cube const& cube) {
                (void) cube;
                return 1;
            })
        : cost_fn(cost_fn) {}

    bool
    is_totally_symmetric(kitty::dynamic_truth_table const& function) const {
        const auto num_vars = function.num_vars();
        for (auto i = 0; i < num_vars; ++i) {
            for (auto j = i + 1; j < num_vars; ++j) {
                if (!kitty::is_symmetric_in(function, i, j)) {
                    return false;
                }
            }
        }
        return true;
    }

    template <class Network>
    void operator()(Network& net,
                    std::vector<tweedledum::qubit_id> const& qubit_map,
                    kitty::dynamic_truth_table const& function) const {
        const auto num_controls = function.num_vars();
        assert(qubit_map.size() == std::size_t(num_controls) + 1u);

        /* synthesize ESOP */
        easy::esop::helliwell_maxsat_statistics stats;
        easy::esop::helliwell_maxsat_params ps;

        easy::esop::esop_t esop;

        /* check if function is already in the cache */
        auto const it = cache.find(function);
        if (it != cache.end()) {
            esop = it->second;
        } else if (is_totally_symmetric(function)) {
            esop = kitty::esop_from_optimum_pkrm(function);
            cache.emplace(function, esop);
        } else {
            auto const& pprm = kitty::esop_from_pprm(function);
            auto const& pkrm = kitty::esop_from_optimum_pkrm(function);

            if (function.num_vars() >= 5 && pkrm.size() >= 8) {
                esop = pkrm;
                cache.emplace(function, esop);
            } else {
                auto const& exact =
                    easy::esop::esop_from_tt<kitty::dynamic_truth_table,
                                             easy::sat2::maxsat_rc2,
                                             easy::esop::helliwell_maxsat>(
                        stats, ps)
                        .synthesize(function, cost_fn);

                auto const pprm_Tcost = easy::esop::T_count(pprm, num_controls);
                auto const pkrm_Tcost = easy::esop::T_count(pkrm, num_controls);
                auto const exact_Tcost =
                    easy::esop::T_count(exact, num_controls);

                auto const min =
                    std::min(exact_Tcost, std::min(pprm_Tcost, pkrm_Tcost));
                esop = (min == exact_Tcost ? exact
                                           : (min == pkrm_Tcost ? pkrm : pprm));
                cache.emplace(function, esop);
            }
        }

        std::vector<tweedledum::qubit_id> target = {qubit_map.back()};
        for (auto const& cube : esop) {
            std::vector<tweedledum::qubit_id> controls, negations;
            auto bits = cube._bits;
            auto mask = cube._mask;
            for (auto v = 0; v < num_controls; ++v) {
                if (mask & 1) {
                    controls.push_back(tweedledum::qubit_id(qubit_map[v]));
                    if (!(bits & 1)) {
                        negations.push_back(tweedledum::qubit_id(qubit_map[v]));
                    }
                }
                bits >>= 1;
                mask >>= 1;
            }
            for (auto n : negations) {
                net.add_gate(td::gate::pauli_x, n);
            }
            net.add_gate(td::gate::mcx, controls, target);
            for (auto n : negations) {
                net.add_gate(td::gate::pauli_x, n);
            }
        }
    }

  protected:
    std::function<int(kitty::cube)> const& cost_fn;
    mutable std::unordered_map<kitty::dynamic_truth_table, easy::esop::esop_t,
                               kitty::hash<kitty::dynamic_truth_table>>
        cache;
};

} // namespace caterpillar
