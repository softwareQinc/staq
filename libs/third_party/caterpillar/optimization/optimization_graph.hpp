/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <algorithm>
#include <cmath>
#include <kitty/cube.hpp>
#include <vector>

#include "../details/utils.hpp"

namespace caterpillar {
inline int count_set(const uint16_t bits) { return __builtin_popcount(bits); }

inline int num_variables(const std::vector<kitty::cube>& esop) {
    int16_t var_mask;

    for (auto& cube : esop)
        var_mask = var_mask | cube._mask;

    return count_set(var_mask);
}

struct optimized_esop {
    std::vector<kitty::cube> cubes;
    std::vector<int> pairing;

  public:
    optimized_esop() = default;
    optimized_esop(std::vector<kitty::cube> cubes, std::vector<int> pairing)
        : cubes(cubes), pairing(pairing) {}

    uint get_num_pairs = pairing.size();
    void print() {
        std::cout << "Ordered cubes for optimized esop:" << std::endl;
        for (auto cube : cubes) {
            cube.print(5);
            std::cout << std::endl;
        }
        for (auto type : pairing) {
            std::cout << type << " ";
        }
        std::cout << std::endl;
    }
};

class optimization_graph {

    struct edge {
        int from;
        int to;
        uint weight = 0;
        bool matched = false;
        int type;

        edge(int from, int to, uint weight, int type)
            : from(from), to(to), weight(weight), type(type) {}

        void match() { matched = true; }
    };

    std::vector<kitty::cube> esop;
    std::vector<edge> edges;
    int num_var;

    int get_gain_first_property(kitty::cube f, kitty::cube s) {
        /* whether two esop have some common controls, and one of them has a
           unique control that is not present in the other --> we have a gain
           only if there are some independent controls */
        if ((((f._mask & s._mask) & (f._bits ^ s._bits)) == 0) &&
            ((f._mask & s._mask) != 0)) {
            auto a_notb = f._mask & (~s._mask);
            auto b_nota = (~f._mask) & s._mask;

            int old_cost = detail::t_cost(f.num_literals(), num_var) +
                           detail::t_cost(s.num_literals(), num_var);
            if (count_set(a_notb) == 1) {
                return old_cost -
                       (2 * detail::t_cost(count_set(b_nota), num_var) +
                        detail::t_cost(f.num_literals(), num_var));
            }

            if (count_set(b_nota) == 1) {

                return old_cost -
                       (2 * detail::t_cost(count_set(a_notb), num_var) +
                        detail::t_cost(s.num_literals(), num_var));
            }
            return 0;
        } else
            return 0;
    }

    int get_gain_second_property(kitty::cube f, kitty::cube s) {
        /* same controls but different polarities */
        if ((f._mask == s._mask) && (f._bits ^ s._bits) != 0) {
            auto old_cost = 2 * detail::t_cost(f.num_literals(), num_var);
            return old_cost - (detail::t_cost(f.num_literals() - 1, num_var));
        }
        return 0;
    }

  public:
    void print() {
        for (auto edge : edges)
            std::cout << edge.from << " -- " << edge.to << " : " << edge.weight
                      << std::endl;
    }

    optimized_esop match_properties() {
        std::vector<int> m_nodes;
        std::vector<kitty::cube> cubes;
        std::vector<int> pairing;

        std::sort(edges.begin(), edges.end(),
                  [](edge a, edge b) { return a.weight > b.weight; });

        for (auto& edge : edges) {
            if ((std::find(m_nodes.begin(), m_nodes.end(), edge.from) ==
                 m_nodes.end()) &&
                (std::find(m_nodes.begin(), m_nodes.end(), edge.to) ==
                 m_nodes.end())) {
                edge.match();
                m_nodes.push_back(edge.from);
                m_nodes.push_back(edge.to);
            }
        }

        for (auto& edge : edges) {
            if (edge.matched) {

                cubes.push_back(esop[edge.from]);
                cubes.push_back(esop[edge.to]);
                pairing.push_back(edge.type);
            }
        }

        for (uint i = 0; i < esop.size(); i++) {
            if (std::find(m_nodes.begin(), m_nodes.end(), i) == m_nodes.end())
                cubes.push_back(esop[i]);
        }
        return optimized_esop(cubes, pairing);
    }

    optimization_graph(std::vector<kitty::cube> esop) : esop(esop) {
        /* Each graph corresponds to an esop, each node to a cube,
            each edge to pairing cubes with weight=costgain*/

        num_var = num_variables(esop);

        for (unsigned i = 0; i < esop.size() - 1; ++i) {
            auto cube = esop[i];
            for (unsigned j = i + 1; j < esop.size(); ++j) {
                auto paired = esop[j];
                if (paired != cube) {
                    auto gfp = get_gain_first_property(cube, paired);
                    auto gsp = get_gain_second_property(cube, paired);
                    if (gfp != 0)
                        edges.push_back(edge(i, j, gfp, 1));
                    if (gsp != 0)
                        edges.push_back(edge(i, j, gsp, 2));
                }
            }
        }
    }
};
} // namespace caterpillar