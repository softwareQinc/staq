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
    device(std::string name, uint32_t n, std::vector<std::vector<bool> >& dag,
           std::vector<double>& sq_fi, std::vector<std::vector<double> >& tq_fi)
      : name_(name)
      , qubits_(n)
      , couplings_(dag)
      , single_qubit_fidelities(sq_fi)
      , coupling_fidelities(tq_fi)
    {}

    std::string name_;
    uint32_t qubits_;
    std::vector<std::vector<bool> > couplings_;

  private:
    std::vector<double> single_qubit_fidelities_;
    std::vector<std::vector<double> > coupling_fidelities_;

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
