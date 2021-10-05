/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2021 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * \file mapping/device.hpp
 * \brief Representation & tools for restricted device topologies
 */

#pragma once

#include "qasmtools/ast/var.hpp"

#include <limits>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

namespace staq {
namespace mapping {

using json = nlohmann::json;
namespace ast = qasmtools::ast;

using layout = std::unordered_map<ast::VarAccess, int>;
using path = std::list<int>;
using coupling = std::pair<int, int>;
using cmp_couplings = std::function<bool(std::pair<coupling, double>,
                                         std::pair<coupling, double>)>;
using spanning_tree = std::list<std::pair<int, int>>;

static double FIDELITY_1 = 1 - std::numeric_limits<double>::epsilon();

/**
 * \class staq::mapping::Device
 * \brief Class representing physical devices with restricted topologies & gate
 * fidelities
 *
 * A device is used to store information about the number and arrangement of
 * qubits in a hypothetical or real physical device. Devices may or may not
 * include approximate fidelities for single-qubit and multi-qubit gates, but
 * every device at least contains a number of qubits and a digraph giving the
 * allowable CNOT gates. At the moment, all two-qubit gates are CNOT gates.
 *
 * The device class also allows computation of shortest paths between vertices
 * and [Steiner trees](https://en.wikipedia.org/wiki/Steiner_tree_problem) for
 * solving mapping problems.
 */
class Device {
  public:
    /** @name Constructors */
    /**@{*/
    /** \brief Default constructor */
    Device() = default;
    /**
     * \brief Construct a device from a coupling graph
     * \param name A name for the device
     * \param n The number of qubits
     * \param dag A digraph, given as a Boolean adjacency matrix
     */
    Device(std::string name, int n, const std::vector<std::vector<bool>>& dag)
        : name_(name), qubits_(n), couplings_(dag),
          single_qubit_fidelities_(n, FIDELITY_1),
          coupling_fidelities_(n, std::vector<double>(n, FIDELITY_1)) {}
    /**
     * \brief Construct a device from a coupling graph
     * \param name A name for the device
     * \param n The number of qubits
     * \param dag A digraph, given as a Boolean adjacency matrix
     * \param sq_fi A vector of average single-qubit gate fidelities for each
     * qubit \param tq_fi A matrix of average two-qubit gate fidelities for each
     * directed pair
     */
    Device(std::string name, int n, const std::vector<std::vector<bool>>& dag,
           const std::vector<double>& sq_fi,
           const std::vector<std::vector<double>>& tq_fi)
        : name_(name), qubits_(n), couplings_(dag),
          single_qubit_fidelities_(sq_fi), coupling_fidelities_(tq_fi) {}
    /**@}*/

    std::string name_;
    int qubits_;

    /**
     * \brief Whether the device allows a CNOT between two qubits
     * \param i The control qubit
     * \param j The target qubit
     * \return True if the device admits a CNOT between qubits i and j
     */
    bool coupled(int i, int j) {
        if (0 <= i && i < qubits_ && 0 <= j && j < qubits_)
            return couplings_[i][j];
        else
            throw std::out_of_range("Qubit(s) not in range");
    }

    /**
     * \brief Get the single-qubit gate fidelity at a qubit
     * \param i The qubit
     * \return The fidelity as a double precision float
     */
    double sq_fidelity(int i) {
        if (0 <= i && i < qubits_)
            return single_qubit_fidelities_[i];
        else
            throw std::out_of_range("Qubit not in range");
    }
    /**
     * \brief Get the two-qubit gate fidelity at a coupling
     * \param i The control qubit
     * \param j The target qubit
     * \return The fidelity as a double precision float
     */
    double tq_fidelity(int i, int j) {
        if (coupled(i, j))
            return coupling_fidelities_[i][j];
        else
            throw std::logic_error("Qubit not coupled");
    }

    /**
     * \brief Get a shortest path between two qubits
     *
     * Paths are represented as a list of qubits indices
     *
     * \param i The control qubit
     * \param j The target qubit
     * \return A shortest (or highest fidelity) path between qubits i and j
     */
    path shortest_path(int i, int j) {
        compute_shortest_paths();
        path ret{i};

        if (shortest_paths[i][j] == qubits_) {
            return ret;
        }

        while (i != j) {
            i = shortest_paths[i][j];
            ret.push_back(i);
        }

        return ret;
    }

    /**
     * \brief Get the distance of a shortest path between two qubits
     * \param i The control qubit
     * \param j The target qubit
     * \return The length of a shortest path between qubits i and j
     */
    int distance(int i, int j) {
        compute_shortest_paths();

        if (shortest_paths[i][j] == qubits_) {
            return -1;
        }

        int ret = 0;
        while (i != j) {
            i = shortest_paths[i][j];
            ++ret;
        }

        return ret;
    }

    /**
     * \brief Get a list of all edges in the coupling digraph
     * \note Couplings are ordered in decreasing fidelity.
     * \return A set of (coupling, fidelity) pairs
     */
    std::set<std::pair<coupling, double>, cmp_couplings> couplings() {
        // Sort in order of decreasing coupling fidelity
        cmp_couplings cmp = [](std::pair<coupling, double> a,
                               std::pair<coupling, double> b) {
            if (a.second == b.second)
                return a.first < b.first;
            else
                return a.second > b.second;
        };

        std::set<std::pair<coupling, double>, cmp_couplings> ret(cmp);
        for (auto i = 0; i < qubits_; i++) {
            for (auto j = 0; j < qubits_; j++) {
                if (couplings_[i][j]) {
                    ret.insert(std::make_pair(std::make_pair(i, j),
                                              coupling_fidelities_[i][j]));
                }
            }
        }

        return ret;
    }

    /**
     * \brief Get an approximation to a minimal Steiner tree
     *
     * Given a set of terminal nodes and a root node in the coupling graph,
     * attempts to find a minimal weight set of edges connecting the root to
     * each terminal.
     *
     * \param terminals A list of terminal qubits to be connected
     * \param root A root for the Steiner tree
     * \return A spanning tree represented as a list of edges
     */
    spanning_tree steiner(std::list<int> terminals, int root) {
        compute_shortest_paths();

        spanning_tree ret;

        // Internal data structures
        std::vector<double> vertex_cost(qubits_);
        std::vector<int> edge_in(qubits_);
        std::set<int> in_tree{root};

        auto min_node = terminals.end();
        for (auto it = terminals.begin(); it != terminals.end(); it++) {
            vertex_cost[*it] = dist[root][*it];
            edge_in[*it] = root;
            if (min_node == terminals.end() ||
                (vertex_cost[*it] < vertex_cost[*min_node])) {
                min_node = it;
            }
        }

        // Algorithm proper
        while (min_node != terminals.end()) {
            auto current = *min_node;
            terminals.erase(min_node);
            auto new_nodes = add_to_tree(
                ret, shortest_path(edge_in[current], current), in_tree);
            in_tree.insert(new_nodes.begin(), new_nodes.end());

            // Update costs, edges, and find new minimum edge
            min_node = terminals.end();
            for (auto it = terminals.begin(); it != terminals.end(); it++) {
                for (auto node : new_nodes) {
                    if (dist[node][*it] < vertex_cost[*it]) {
                        vertex_cost[*it] = dist[node][*it];
                        edge_in[*it] = node;
                    }
                }
                if (min_node == terminals.end() ||
                    (vertex_cost[*it] < vertex_cost[*min_node])) {
                    min_node = it;
                }
            }
        }

        return ret;
    }

    /**
     * \brief Print the layout information
     *
     * Prints the physical layout of a circuit on the device
     *
     * \param l The physical layout
     * \param os The output stream
     * \param pref An optional prefix
     * \param f An optional permutation
     */
    void print_layout(layout& l, std::ostream& os, std::string pref = "",
                      std::optional<std::map<int, int>> f = std::nullopt) {
        std::unordered_map<int, ast::VarAccess> invmap;
        for (auto it = l.begin(); it != l.end(); it++) {
            invmap.insert({it->second, it->first});
        }

        os << pref << "Mapped to device \"" << name_ << "\"\n";
        os << pref << "Qubits: " << qubits_ << "\n";
        os << pref << "Layout (physical --> virtual):\n";
        if (f) {
            std::map<int, int> invperm;
            for (auto it = f->begin(); it != f->end(); it++) {
                invperm.insert({it->second, it->first});
            }

            for (int i = 0; i < qubits_; i++) {
                os << pref << "\tq[" << i << "] --> ";
                auto it = invmap.find(invperm[i]);
                if (it != invmap.end())
                    os << it->second;
                os << "\n";
            }
            os << "\n";
        } else {
            for (int i = 0; i < qubits_; i++) {
                os << pref << "\tq[" << i << "] --> ";
                auto it = invmap.find(i);
                if (it != invmap.end())
                    os << it->second;
                os << "\n";
            }
        }
    }

    /**
     * \brief Serialize to JSON
     */
    std::string to_json() {
        json js;
        js["name"] = name_;
        for (int i = 0; i < qubits_; i++) {
            js["qubits"].push_back(
                single_qubit_fidelities_[i] == FIDELITY_1
                    ? json{{"id", i}}
                    : json{{"id", i},
                           {"fidelity", single_qubit_fidelities_[i]}});
            for (int j = 0; j < qubits_; j++) {
                if (i != j && couplings_[i][j]) {
                    js["couplings"].push_back(
                        coupling_fidelities_[i][j] == FIDELITY_1
                            ? json{{"control", i}, {"target", j}}
                            : json{{"control", i},
                                   {"target", j},
                                   {"fidelity", coupling_fidelities_[i][j]}});
                }
            }
        }
        return js.dump(2);
    }

  private:
    std::vector<std::vector<bool>>
        couplings_; ///< The adjacency matrix of the device topology
    std::vector<double>
        single_qubit_fidelities_; ///< The fidelities of single-qubit gates
    std::vector<std::vector<double>>
        coupling_fidelities_; ///< The fidelities of two-qubit gates

    /** @name All-pairs-shortest-paths */
    /**@{*/
    std::vector<std::vector<double>>
        dist; ///< Distances returned by Floyd-Warshall
    std::vector<std::vector<int>>
        shortest_paths; ///< Matrix return by Floyd-Warshall
    /**@}*/

    /**
     * \brief Floyd-Warshall all-pairs-shortest-paths algorithm
     * \note Assigns result to dist and shortest_paths
     */
    void compute_shortest_paths() {
        if (dist.empty() || shortest_paths.empty()) {
            // Initialize
            dist = std::vector<std::vector<double>>(
                qubits_, std::vector<double>(qubits_));
            shortest_paths = std::vector<std::vector<int>>(
                qubits_, std::vector<int>(qubits_));

            // All-pairs shortest paths
            for (auto i = 0; i < qubits_; i++) {
                for (auto j = 0; j < qubits_; j++) {
                    if (i == j) {
                        dist[i][j] = 0;
                        shortest_paths[i][j] = j;
                    } else if (couplings_[i][j]) {
                        dist[i][j] = -std::log(coupling_fidelities_[i][j]);
                        shortest_paths[i][j] = j;
                    } else if (couplings_[j][i]) { // Since swaps are the same
                                                   // cost either direction
                        dist[i][j] = -std::log(coupling_fidelities_[j][i]);
                        shortest_paths[i][j] = j;
                    } else {
                        dist[i][j] =
                            -std::log(0.0000000001); // Effectively infinite
                        shortest_paths[i][j] = qubits_;
                    }
                }
            }

            for (auto k = 0; k < qubits_; k++) {
                for (auto i = 0; i < qubits_; i++) {
                    for (auto j = 0; j < qubits_; j++) {
                        if (dist[i][j] > (dist[i][k] + dist[k][j])) {
                            dist[i][j] = dist[i][k] + dist[k][j];
                            shortest_paths[i][j] = shortest_paths[i][k];
                        }
                    }
                }
            }
        }
    }

    /**
     * \brief Adds a path to a spanning tree
     *
     * Inserts a path into a spanning tree so that edges are not duplicated, and
     * moreover maintains the spanning tree property & the topological order on
     * the tree
     *
     * \param s_tree The input spanning tree
     * \param p Const reference to the path to be inserted
     * \param in_tree The set of nodes already in the tree
     * \return The set of nodes in the resulting spanning tree
     */
    std::set<int> add_to_tree(spanning_tree& s_tree, const path& p,
                              const std::set<int>& in_tree) {
        std::set<int> ret;

        int next = -1;
        auto insert_iter = s_tree.end();
        for (auto it = p.rbegin(); it != p.rend(); it++) {
            // If we're not at the endpoint, insert the edge
            if (next != -1) {
                s_tree.insert(insert_iter, std::make_pair(*it, next));
                --insert_iter;
            }
            next = *it;
            ret.insert(*it);

            // If the current node is already in the tree, we're done
            if (in_tree.find(*it) != in_tree.end())
                break;
        }

        return ret;
    }
};

/**
 * \brief JSON deserialization of Device object
 * The JSON object should have:
 * - name: string
 * - qubits: list of {{id: int}, optional {fidelity: double}}
 * - couplings: list of {{control: int}, {target: int}, optional {fidelity:
 * double}} Unspecified fidelities are set to a default value
 */
inline Device parse_json(std::string fname) {
    std::ifstream ifs(fname);
    json j = json::parse(ifs);

    std::string name = j["name"];
    int n = j["qubits"].size();
    std::vector<std::vector<bool>> dag(n, std::vector<bool>(n));
    std::vector<double> sq_fi(n);
    std::vector<std::vector<double>> tq_fi(n, std::vector<double>(n));

    for (json& qubit : j["qubits"]) {
        int id = qubit["id"];
        if (id < 0 || id >= n) {
            throw std::logic_error("Qubit(s) not in range");
        }
        if (sq_fi[id] != 0) {
            throw std::logic_error("Duplicate qubit id");
        }
        auto it = qubit.find("fidelity");
        if (it != qubit.end())
            sq_fi[id] = *it;
        else
            sq_fi[id] = FIDELITY_1;
    }
    for (json& coupling : j["couplings"]) {
        int x = coupling["control"];
        int y = coupling["target"];
        if (x < 0 || x >= n || y < 0 || y >= n) {
            throw std::logic_error("Qubit(s) not in range");
        }
        if (x == y) {
            throw std::logic_error("Qubit can't be coupled with itself");
        }
        if (dag[x][y]) {
            throw std::logic_error("Duplicate coupling");
        }
        dag[x][y] = true;
        auto it = coupling.find("fidelity");
        if (it != coupling.end())
            tq_fi[x][y] = *it;
        else
            tq_fi[x][y] = FIDELITY_1;
    }
    return Device(name, n, dag, sq_fi, tq_fi);
}

/** \brief Generates a fully connected device with a given number of qubits */
inline Device fully_connected(uint32_t n) {
    auto tmp = std::vector<std::vector<bool>>(n, std::vector<bool>(n, true));
    for (uint32_t i = 0; i < n; i++) {
        tmp[i][i] = false;
    }

    return Device("Fully connected device", n, tmp);
}

} // namespace mapping
} // namespace staq
