/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

#include <vector>
#include <map>
#include <list>

namespace synthewareQ {
namespace mapping {

  using layout = std::map<std::pair<std::string_view, size_t>, size_t>;
  using path   = std::list<size_t>;

  /* \brief! Definition of physical devices for efficient mapping */
  class device {
  public:
    device(std::string name, size_t n, const std::vector<std::vector<bool> >& dag)
      : name_(name)
      , qubits_(n)
      , couplings_(dag)
    {
      single_qubit_fidelities_.resize(n);
      for (auto i = 0; i < n; i++) {
        single_qubit_fidelities_[i] = 1.0;
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
    {}

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

  private:
    std::vector<std::vector<bool> > couplings_;
    std::vector<double> single_qubit_fidelities_;
    std::vector<std::vector<double> > coupling_fidelities_;

    std::vector<std::vector<size_t> > shortest_paths;

    // Floyd-Warshall, since it's simple to implement and devices are not currently that big
    void compute_shortest_paths() {
      if (shortest_paths.empty()) {
        // Initialize
        shortest_paths = std::vector<std::vector<size_t> >(qubits_, std::vector<size_t>(qubits_));

        // All-pairs shortest paths
        std::vector<std::vector<double> > dist(qubits_, std::vector<double>(qubits_));
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

    path shortest_path(size_t i, size_t j) {
      path ret;
      
      if (shortest_paths[i][j] == qubits_) {
        return ret;
      }

      while(i != j) {
        i = shortest_paths[i][j];
        ret.push_back(i);
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
    

}
}
