/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*------------------------------------------------------------------------------------------------*/
#pragma once
#include <tweedledum/gates/gate_base.hpp>
#include <tweedledum/networks/netlist.hpp>
#include <tweedledum/networks/io_id.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include <vector>

namespace caterpillar {

namespace td = tweedledum;

class stg_gate : public td::gate_base {

  public:
    stg_gate(gate_base const& op, td::io_id target) : td::gate_base(op) {
        assert(is_single_qubit());
        _targets.push_back(target);
    }

    stg_gate(gate_base const& op, td::io_id control, td::io_id target)
        : gate_base(op) {
        assert(is_double_qubit());
        _controls.push_back(control);
        _targets.push_back(target);
    }

    stg_gate(gate_base const& op, std::vector<td::io_id> const& controls,
             std::vector<td::io_id> const& targets)
        : td::gate_base(op), _controls(controls), _targets(targets) {}

    stg_gate(kitty::dynamic_truth_table const& function,
             std::vector<td::io_id> const& controls, td::io_id target)
        : gate_base(td::gate_lib::num_defined_ops), _function(function),
          _controls(controls) {
        _targets.push_back(target);
    }

    bool is_unitary_gate() const {
        return td::gate_base::is_gate() ||
               operation() == td::gate_lib::num_defined_ops;
    }

    uint32_t num_controls() const { return _controls.size(); }

    uint32_t num_targets() const { return _targets.size(); }

    template <typename Fn>
    void foreach_control(Fn&& fn) const {
        for (auto c : _controls) {
            fn(c);
        }
    }

    template <typename Fn>
    void foreach_target(Fn&& fn) const {
        for (auto t : _targets) {
            fn(t);
        }
    }

  private:
    /*! \brief control function of a single-target gate */
    kitty::dynamic_truth_table _function;

    /*! \brief set control qubits in the network. */
    std::vector<td::io_id> _controls{};

    /*! \brief set of target qubits in the network. */
    std::vector<td::io_id> _targets{};
};

} // namespace caterpillar
