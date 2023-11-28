/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_lib.hpp"
#include "../traits.hpp"
#include "immutable_view.hpp"

#include <array>
#include <fmt/format.h>
#include <iostream>

namespace tweedledum {

/*! \brief
 *
 * **Required gate functions:**
 *
 * **Required network functions:**
 *
 */
template <typename Network>
class stats_view : public immutable_view<Network> {
  public:
    using gate_type = typename Network::gate_type;
    using node_type = typename Network::node_type;
    using node_ptr_type = typename Network::node_ptr_type;
    using storage_type = typename Network::storage_type;

    explicit stats_view(Network& ntk) : immutable_view<Network>(ntk) {
        num_gates_per_op.fill(0);
        compute_statistics();
    }

    uint32_t num_gates(gate_lib operation) {
        const auto op = static_cast<uint32_t>(operation);
        return num_gates_per_op[op];
    }

  private:
    void compute_statistics() {
        this->foreach_gate([&](auto const& node) {
            const auto op = static_cast<uint32_t>(node.gate.operation());
            num_gates_per_op[op] += 1;
        });
    }

  private:
    static constexpr auto num_defined_ops =
        static_cast<uint32_t>(gate_lib::num_defined_ops);
    std::array<uint32_t, num_defined_ops> num_gates_per_op;
};

} // namespace tweedledum
