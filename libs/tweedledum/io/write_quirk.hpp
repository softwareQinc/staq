/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_base.hpp"
#include "../gates/gate_lib.hpp"
#include "../networks/io_id.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ostream>
#include <string>
#include <vector>

namespace tweedledum {
namespace detail {

template <class Iterator, class MapFn, class JoinFn>
auto map_and_join(Iterator begin, Iterator end, MapFn&& map_fn,
                  JoinFn&& join_fn) {
    return std::accumulate(
        begin + 1, end, map_fn(*begin),
        [&](auto const& a, auto const& v) { return join_fn(a, map_fn(v)); });
}

class quirk_json_builder {
  public:
    quirk_json_builder(uint32_t num_qubits) : num_qubits_(num_qubits) {
        new_column();
    }

    void add_gate(std::string const& op, io_id target) {
        if (columns_.back()[target] != "1") {
            new_column();
        }
        columns_.back()[target] = op;
    }

    void add_gate(std::string const& op, io_id control, io_id target) {
        if (!is_last_column_empty()) {
            new_column();
        }
        columns_.back()[control] = control.is_complemented() ? "◦" : "•";
        columns_.back()[target] = op;
        new_column();
    }

    void add_swap(io_id target0, io_id target1) {
        if (!is_last_column_empty()) {
            new_column();
        }
        columns_.back()[target0] = "Swap";
        columns_.back()[target1] = "Swap";
        new_column();
    }

    void add_gate(std::string const& op, std::vector<io_id> controls,
                  std::vector<io_id> targets) {
        if (!is_last_column_empty()) {
            new_column();
        }
        for (auto control : controls) {
            columns_.back()[control] = control.is_complemented() ? "◦" : "•";
        }
        for (auto target : targets) {
            columns_.back()[target] = op;
        }
        new_column();
    }

    friend std::ostream& operator<<(std::ostream& out,
                                    quirk_json_builder builder) {
        const auto join_with_comma = [](std::vector<std::string> const& v) {
            return "[" +
                   map_and_join(v.begin(), v.end(),
                                [](auto const& s) {
                                    return s == "1" ? "1" : "\"" + s + "\"";
                                },
                                [](auto const& a, auto const& b) {
                                    return a + "," + b;
                                }) +
                   "]";
        };
        out << map_and_join(
            builder.columns_.begin(), builder.columns_.end(),
            [&](const auto& v) { return join_with_comma(v); },
            [&](auto const& a, auto const& b) { return a + "," + b; });

        return out;
    }

  private:
    void new_column() { columns_.emplace_back(num_qubits_, "1"); }

    bool is_last_column_empty() {
        auto element =
            std::find_if(columns_.back().begin(), columns_.back().end(),
                         [](auto const& s) { return s != "1"; });
        return element == columns_.back().end();
    }

  private:
    uint32_t num_qubits_;
    std::vector<std::vector<std::string>> columns_;
};

} // namespace detail

/*! \brief Writes network in Quirk format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required gate functions:**
 * - `op`
 * - `foreach_control`
 * - `foreach_target`
 *
 * **Required network functions:**
 * - `foreach_gate`
 * - `num_qubits`
 *
 * \param network Network
 * \param os Output stream
 */
template <class Network>
void write_quirk(Network const& network, std::ostream& os = std::cout) {
    if (network.num_gates() == 0) {
        return;
    }

    detail::quirk_json_builder builder(network.num_qubits());
    network.foreach_gate([&](auto const& node) {
        auto const& gate = node.gate;
        switch (gate.operation()) {
            default:
                std::cerr << "[w] unsupported gate type\n";
                return true;

            case gate_lib::hadamard:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("H", qid); });
                break;

            case gate_lib::pauli_x:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("X", qid); });
                break;

            case gate_lib::pauli_z:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("Z", qid); });
                break;

            case gate_lib::phase:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("Z^%C2%BD", qid); });
                break;

            case gate_lib::phase_dagger:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("Z^-%C2%BD", qid); });
                break;

            case gate_lib::t:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("Z^%C2%BC", qid); });
                break;

            case gate_lib::t_dagger:
                gate.foreach_target(
                    [&](auto qid) { builder.add_gate("Z^-%C2%BC", qid); });
                break;

            case gate_lib::cx:
                gate.foreach_control([&](auto qid_control) {
                    gate.foreach_target([&](auto qid_target) {
                        builder.add_gate("X", qid_control, qid_target);
                    });
                });
                break;

            case gate_lib::mcx: {
                std::vector<io_id> controls;
                std::vector<io_id> targets;
                gate.foreach_control(
                    [&](auto control) { controls.push_back(control); });
                gate.foreach_target(
                    [&](auto target) { targets.push_back(target); });
                builder.add_gate("X", controls, targets);
            } break;

            case gate_lib::cz:
                gate.foreach_control([&](auto qid_control) {
                    gate.foreach_target([&](auto qid_target) {
                        builder.add_gate("Z", qid_control, qid_target);
                    });
                });
                break;

            case gate_lib::mcz: {
                std::vector<io_id> controls;
                std::vector<io_id> targets;
                gate.foreach_control(
                    [&](auto control) { controls.push_back(control); });
                gate.foreach_target(
                    [&](auto target) { targets.push_back(target); });
                builder.add_gate("Z", controls, targets);
            } break;

            case gate_lib::swap: {
                std::vector<io_id> targets;
                gate.foreach_target(
                    [&](auto target) { targets.push_back(target); });
                builder.add_swap(targets.at(0), targets.at(1));
            } break;
        }
        return true;
    });

    os << "\"cols\":[" << builder << "]\n";
}

/*! \brief Writes network in Quirk format into a file
 *
 * **Required gate functions:**
 * - `op`
 * - `foreach_control`
 * - `foreach_target`
 *
 * **Required network functions:**
 * - `foreach_gate`
 * - `num_qubits`
 *
 * \param network Network
 * \param filename Filename
 */
template <class Network>
void write_quirk(Network const& network, std::string const& filename) {
    std::ofstream os(filename.c_str(), std::ofstream::out);
    write_quirk(network, os);
}

} // namespace tweedledum
