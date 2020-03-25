/*
 * This file is part of staq.
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

#pragma once

/**
 * \file mapping/device.hpp
 * \brief Representation & tools for restricted device topologies
 */

#include "ast/var.hpp"

#include <vector>
#include <unordered_map>
#include <list>
#include <set>

#include <functional>
#include <queue>

#include <iostream>

namespace staq {
namespace mapping {

using layout = std::unordered_map<ast::VarAccess, int>;
using path = std::list<int>;
using coupling = std::pair<int, int>;
using cmp_couplings = std::function<bool(std::pair<coupling, double>,
                                         std::pair<coupling, double>)>;
using spanning_tree = std::list<std::pair<int, int>>;

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
    /** \brief Empty constructor */
    Device() {}
    /**
     * \brief Construct a device from a coupling graph
     * \param name A name for the device
     * \param n The number of qubits
     * \param dag A digraph, given as a Boolean adjacency matrix
     */
    Device(std::string name, int n, const std::vector<std::vector<bool>>& dag)
        : name_(name), qubits_(n), couplings_(dag) {
        single_qubit_fidelities_.resize(n);
        coupling_fidelities_.resize(n);
        for (auto i = 0; i < n; i++) {
            single_qubit_fidelities_[i] = 0.99;
            coupling_fidelities_[i].resize(n);
            for (auto j = 0; j < n; j++) {
                coupling_fidelities_[i][j] = 0.99;
            }
        }
    }
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
                        dist[i][j] = 1.0 - coupling_fidelities_[i][j];
                        shortest_paths[i][j] = j;
                    } else if (couplings_[j][i]) { // Since swaps are the same
                                                   // cost either direction
                        dist[i][j] = 1.0 - coupling_fidelities_[j][i];
                        shortest_paths[i][j] = j;
                    } else {
                        dist[i][j] = 10.0; // Effectively infinite
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
 * \var Device Rigetti-Agave
 * \brief The Rigetti 8 qubit Agave qpu
 *
 * Qubits are arranged like so:
   \verbatim
   1 -- 0 -- 7
   |         |
   2         6
   |         |
   3 -- 4 -- 5
   \endverbatim
 */
static Device agave("Rigetti Agave", 8,
                    {
                        {0, 1, 0, 0, 0, 0, 0, 1},
                        {1, 0, 1, 0, 0, 0, 0, 0},
                        {0, 1, 0, 1, 0, 0, 0, 0},
                        {0, 0, 1, 0, 1, 0, 0, 0},
                        {0, 0, 0, 1, 0, 1, 0, 0},
                        {0, 0, 0, 0, 1, 0, 1, 0},
                        {0, 0, 0, 0, 0, 1, 0, 1},
                        {1, 0, 0, 0, 0, 0, 1, 0},
                    },
                    {0.957, 0.951, 0.982, 0.970, 0.969, 0.962, 0.969, 0.932},
                    {
                        {0, 0.92, 0, 0, 0, 0, 0, 0.92},
                        {0.91, 0, 0.91, 0, 0, 0, 0, 0},
                        {0, 0.82, 0, 0.82, 0, 0, 0, 0},
                        {0, 0, 0.87, 0, 0.87, 0, 0, 0},
                        {0, 0, 0, 0.67, 0, 0.67, 0, 0},
                        {0, 0, 0, 0, 0.93, 0, .093, 0},
                        {0, 0, 0, 0, 0, 0.93, 0, 0.93},
                        {0.91, 0, 0, 0, 0, 0, 0.91, 0},
                    });

/**
 * \var Device Rigetti-Aspen-4
 * \brief A 16-qubit Rigetti lattice
 *
 * Qubits are arranged like so:
   \verbatim
     4 -- 3         12 -- 11
    /      \       /        \
   5        2 -- 13          10
   |        |     |          |
   6        1 -- 14          9
    \      /       \        /
     7 -- 0         15 -- 8
   \endverbatim
 */
static Device aspen4("16-qubit Rigetti lattice", 16,
                     {{0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                      {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
                      {0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
                      {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                      {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
                      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0},
                      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0},
                      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
                      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0}});

/**
 * \var Device IBM-Tokyo
 * \brief A 20-qubit IBM device
 *
 * Qubits are arranged like so:
   \verbatim
    0 --  1 --  2 --  3 --  4
    |     |     |     |  X  |
    5 --  6 --  7 --  8 --  9
    |  X  |     |  X  |
   10 -- 11 -- 12 -- 13 -- 14
    |     |  X        |  X  |
   15 -- 16 -- 17    18    19
   \endverbatim
 */
static Device
    tokyo("20 qubit IBM Tokyo device", 20,
          {{0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
           {1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
           {0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
           {0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
           {0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
           {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
           {0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
           {0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
           {0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
           {0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
           {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
           {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0},
           {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0},
           {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1},
           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1},
           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0},
           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0},
           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
           {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}});
/**
 * \var Device IBM-Singapore
 * \brief A 20-qubit IBM device
 *
 * Qubits are arranged like so:
   \verbatim
    0 --  1 --  2 --  4 --  3
          |           |
   12 -- 11 -- 10 --  5 --  6
    |           |           |
   13 -- 14 --  9 --  8 --  7
          |           |
   15 -- 16 -- 17 -- 18 -- 19
   \endverbatim
 */
static Device
    singapore("20 qubit IBM Singapore device", 20,
              {{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
               {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0},
               {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0}});

/**
 * \var Device square_9q
 * \brief A 9 qubit square lattice
 *
 * Qubits are arranged like so:
   \verbatim
   0 -- 1 -- 2
   |    |    |
   5 -- 4 -- 3
   |    |    |
   6 -- 7 -- 8
   \endverbatim
 */
static Device square_9q("9 qubit square lattice", 9,
                        {
                            {0, 1, 0, 0, 0, 1, 0, 0, 0},
                            {1, 0, 1, 0, 1, 0, 0, 0, 0},
                            {0, 1, 0, 1, 0, 0, 0, 0, 0},
                            {0, 0, 1, 0, 1, 0, 0, 0, 1},
                            {0, 1, 0, 1, 0, 1, 0, 1, 0},
                            {1, 0, 0, 0, 1, 0, 1, 0, 0},
                            {0, 0, 0, 0, 0, 1, 0, 1, 0},
                            {0, 0, 0, 0, 1, 0, 1, 0, 1},
                            {0, 0, 0, 1, 0, 0, 0, 1, 0},
                        });

/** \brief Generates a fully connected device with a given number of qubits */
inline Device fully_connected(uint32_t n) {
    return Device(
        "Fully connected device", n,
        std::vector<std::vector<bool>>(n, std::vector<bool>(n, true)));
}

} // namespace mapping
} // namespace staq
