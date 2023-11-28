/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*------------------------------------------------------------------------------------------------*/
#pragma once

namespace caterpillar::detail {
int t_cost(const int tof_controls, const int lines) {
    switch (tof_controls) {
        case 0u:
        case 1u:
            return 0;

        case 2u:
            return 7;

        case 3u:
            return 16;

        default:
            if (lines - tof_controls - 1 >= (tof_controls - 1) / 2) {
                return 8 * (tof_controls - 1);
            } else {
                return 16 * (tof_controls - 1);
            }
    }
}

template <class TofNetwork>
int count_t_gates(TofNetwork const& netlist) {
    auto T_number = 0u;
    netlist.foreach_cgate([&](const auto& gate) {
        T_number += t_cost(gate.gate.num_controls(), netlist.size());
    });
    return T_number;
};

} // namespace caterpillar::detail