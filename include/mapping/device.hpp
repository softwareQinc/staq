/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

#include <vector>
#include <map>
#include <list>
#include <set>

#include <functional>
#include <queue>

#include <iostream>

namespace synthewareQ {
namespace mapping {

  using layout       = std::map<std::pair<std::string_view, size_t>, size_t>;
  using path         = std::list<size_t>;
  using coupling     = std::pair<size_t, size_t>;
  using spanning_tree = std::list<std::pair<size_t, size_t> >;

  /* \brief! Definition of physical devices for efficient mapping */
  class device {
  public:
    device(std::string name, size_t n, const std::vector<std::vector<bool> >& dag)
      : name_(name)
      , qubits_(n)
      , couplings_(dag)
    {
      single_qubit_fidelities_.resize(n);
      coupling_fidelities_.resize(n);
      for (auto i = 0; i < n; i++) {
        single_qubit_fidelities_[i] = 1.0;
        coupling_fidelities_[i].resize(n);
        for (auto j = 0; j < n; j++) {
          coupling_fidelities_[i][j] = 1.0;
        }
      }
    }
    device(std::string name, size_t n, const std::vector<std::vector<bool> >& dag,
           const std::vector<double>& sq_fi, const std::vector<std::vector<double> >& tq_fi)
      : name_(name)
      , qubits_(n)
      , couplings_(dag)
      , single_qubit_fidelities_(sq_fi)
      , coupling_fidelities_(tq_fi)
    { }

    std::string name_;
    size_t qubits_;

    bool coupled(size_t i, size_t j) {
      if (0 <= i && i < qubits_ && 0 <= j && j < qubits_) return couplings_[i][j];
      else throw std::out_of_range("Qubit(s) not in range");
    }

    double sq_fidelity(size_t i) {
      if (0 <= i && i < qubits_) return single_qubit_fidelities_[i];
      else throw std::out_of_range("Qubit not in range");
    }
    double tq_fidelity(size_t i, size_t j) {
      if (coupled(i, j)) return coupling_fidelities_[i][j];
      else throw std::logic_error("Qubit not coupled");
    }

    path shortest_path(size_t i, size_t j) {
      compute_shortest_paths();
      path ret { i };
      
      if (shortest_paths[i][j] == qubits_) {
        return ret;
      }

      while(i != j) {
        i = shortest_paths[i][j];
        ret.push_back(i);
      }

      return ret;
    }

    std::set<std::pair<coupling, double> > couplings() {
      // Sort in order of decreasing coupling fidelity
      using comparator = std::function<bool(std::pair<coupling, double>, std::pair<coupling, double>)>;
      comparator cmp = [](std::pair<coupling, double> a, std::pair<coupling, double> b) {
        return a.second > b.second;
      };

      std::set<std::pair<coupling, double> > ret;
      for (auto i = 0; i < qubits_; i++) {
        for (auto j = 0; j < qubits_; j++) {
          if (couplings_[i][j]) {
            ret.insert(std::make_pair(std::make_pair(i, j), coupling_fidelities_[i][j]));
          }
        }
      }

      return ret;
    }

    // Returns an approximation to the minimal rooted steiner tree
    spanning_tree steiner(std::list<size_t> terminals, size_t root)
    {
      compute_shortest_paths();

      spanning_tree ret;

      // Internal data structures
      std::vector<double> vertex_cost(qubits_);
      std::vector<size_t> edge_in(qubits_);
      std::set<size_t> in_tree{root};

      auto min_node = terminals.end();
      for (auto it = terminals.begin(); it != terminals.end(); it++) {
        vertex_cost[*it] = dist[root][*it];
        edge_in[*it] = root;
        if (min_node == terminals.end() || (vertex_cost[*it] < vertex_cost[*min_node])) {
          min_node = it;
        }
      }

      // Algorithm proper
      while (min_node != terminals.end()) {
        auto current = *min_node;
        terminals.erase(min_node);
        auto new_nodes = add_to_tree(ret, shortest_path(edge_in[current], current), in_tree);
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
          if (min_node == terminals.end() || (vertex_cost[*it] < vertex_cost[*min_node])) {
            min_node = it;
          }
        }
      }

      return ret;
    }
        
  private:
    std::vector<std::vector<bool> > couplings_;
    std::vector<double> single_qubit_fidelities_;
    std::vector<std::vector<double> > coupling_fidelities_;

    // Utilities computed by all-pairs-shortest-paths, for use getting shortest paths
    // and Steiner trees
    std::vector<std::vector<double> > dist;
    std::vector<std::vector<size_t> > shortest_paths;

    // Floyd-Warshall, since it's simple to implement and devices are not currently that big
    void compute_shortest_paths() {
      if (dist.empty() || shortest_paths.empty()) {
        // Initialize
        dist           = std::vector<std::vector<double> >(qubits_, std::vector<double>(qubits_));
        shortest_paths = std::vector<std::vector<size_t> >(qubits_, std::vector<size_t>(qubits_));

        // All-pairs shortest paths
        for (auto i = 0; i < qubits_; i++) {
          for (auto j = 0; j < qubits_; j++) {
            if (i == j) {
              dist[i][j] = 0;
              shortest_paths[i][j] = j;
            } else if (couplings_[i][j]) {
              dist[i][j] = 1.0 - coupling_fidelities_[i][j];
              shortest_paths[i][j] = j;
            } else if (couplings_[j][i]) { // Since swaps are the same cost either direction
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

    // Adds a path to a spanning tree, maintaining the spanning tree property &
    // the topological order on s_tree
    //
    // Additionally returns the nodes added to the tree
    std::set<size_t> add_to_tree(spanning_tree& s_tree, const path& p, const std::set<size_t>& in_tree) {
      std::set<size_t> ret;

      size_t next = -1;
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
        if (in_tree.find(*it) != in_tree.end()) break;
      }

      return ret;
    }

  };

  device rigetti_8q(
    "Rigetti 8Q",
    8,
    { {0, 1, 0, 0, 0, 0, 0, 1},
      {1, 0, 1, 0, 0, 0, 0, 0},
      {0, 1, 0, 1, 0, 0, 0, 0},
      {0, 0, 1, 0, 1, 0, 0, 0},
      {0, 0, 0, 1, 0, 1, 0, 0},
      {0, 0, 0, 0, 1, 0, 1, 0},
      {0, 0, 0, 0, 0, 1, 0, 1},
      {1, 0, 0, 0, 0, 0, 1, 0},},
    { 0.957, 0.951, 0.982, 0.970, 0.969, 0.962, 0.969, 0.932 },
    { {0, 0.92, 0, 0, 0, 0, 0, 0.92},
      {0.91, 0, 0.91, 0, 0, 0, 0, 0},
      {0, 0.82, 0, 0.82, 0, 0, 0, 0},
      {0, 0, 0.87, 0, 0.87, 0, 0, 0},
      {0, 0, 0, 0.67, 0, 0.67, 0, 0},
      {0, 0, 0, 0, 0.93, 0, .093, 0},
      {0, 0, 0, 0, 0, 0.93, 0, 0.93},
      {0.91, 0, 0, 0, 0, 0, 0.91, 0},}
  );
    
  device square_9q(
    "9 qubit square lattice",
    9,
    { {0, 1, 0, 0, 0, 1, 0, 0, 0},
      {1, 0, 1, 0, 1, 0, 0, 0, 0},
      {0, 1, 0, 1, 0, 0, 0, 0, 0},
      {0, 0, 1, 0, 1, 0, 0, 0, 1},
      {0, 1, 0, 1, 0, 1, 0, 1, 0},
      {1, 0, 0, 0, 1, 0, 1, 0, 0},
      {0, 0, 0, 0, 0, 1, 0, 1, 0},
      {0, 0, 0, 0, 1, 0, 1, 0, 1},
      {0, 0, 0, 1, 0, 0, 0, 1, 0}, }
  );

}
}
