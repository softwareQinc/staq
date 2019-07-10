/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#include <vector>
#include <map>

namespace synthewareQ {
namespace mapping {

  /* \brief! Definition of physical devices for efficient mapping */

  class device {
  public:
    device(std::string name, uint32_t n, std::vector<std::pair<uint32_t, uint32_t> >& dag)
      : name_(name)
      , qubits_(n)
      , couplings_(dag)
    {
      single_qubit_fidelities.resize(n);
      for (auto i = 0; i < n; i++) {
        single_qubit_fidelities[i] = 1.0;
      }

      for (auto pair : couplings) {
        coupling_fidelities[pair] = 1.0;
      }
    }
    device(std::string name, uint32_t n, std::vector<std::pair<uint32_t, uint32_t> >& dag,
           std::vector<double>& sq_fi, std::map<std::pair<uint32_t, uint32_t> >& tq_fi)
      : name_(name)
      , qubits_(n)
      , couplings_(dag)
      , single_qubit_fidelities(sq_fi)
      , coupling_fidelities(tq_fi)
    {}

    std::string name_;
    uint32_t qubits_;
    std::vector<std::pair<uint32_t, uint32_t> > couplings_;

  private:
    std::vector<double> single_qubit_fidelities_;
    std::map<std::pair<uint32_t, uint32_t> > coupling_fidelities_;

  };

  device rigetti_8q(
    "Rigetti 8Q",
    8,
    { {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 0} },
    { 0.957, 0.951, 0.982, 0.970, 0.969, 0.962, 0.969, 0.932 },
    { { {0, 1}, 0.92 },
      { {1, 2}, 0.91 },
      { {2, 3}, 0.82 },
      { {3, 4}, 0.87 },
      { {4, 5}, 0.67 },
      { {5, 6}, 0.93 },
      { {6, 7}, 0.93 },
      { {7, 0}, 0.91 } }
  );
    

}
}
