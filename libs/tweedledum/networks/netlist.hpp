/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../gates/gate_base.hpp"
#include "../utils/foreach.hpp"
#include "detail/storage.hpp"
#include "io_id.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <type_traits>

namespace tweedledum {

/*! \brief
 */
template <typename GateType>
class netlist {
  public:
#pragma region Types and constructors
    using gate_type = GateType;
    using node_type = wrapper_node<gate_type, 1>;
    using node_ptr_type = typename node_type::pointer_type;
    using storage_type = storage<node_type>;

    netlist()
        : storage_(std::make_shared<storage_type>()),
          labels_(std::make_shared<labels_map>()) {}

    explicit netlist(std::string_view name)
        : storage_(std::make_shared<storage_type>(name)),
          labels_(std::make_shared<labels_map>()) {}
#pragma endregion

#pragma region I / O and ancillae qubits
  private:
    auto create_io(bool is_qubit) {
        io_id id(storage_->inputs.size(), is_qubit);
        uint32_t index = storage_->nodes.size();
        gate_type input(gate_base(gate_lib::input), id);
        gate_type output(gate_base(gate_lib::output), id);

        storage_->nodes.emplace_back(input);
        storage_->inputs.emplace_back(index);
        storage_->outputs.emplace_back(output);
        storage_->rewiring_map.push_back(id);
        return id;
    }

  public:
    io_id add_qubit(std::string const& qlabel) {
        auto qid = create_io(true);
        labels_->map(qid, qlabel);
        storage_->num_qubits += 1;
        return qid;
    }

    io_id add_qubit() {
        auto qlabel = fmt::format("q{}", num_qubits());
        return add_qubit(qlabel);
    }

    io_id add_cbit(std::string const& label) {
        auto qid = create_io(false);
        labels_->map(qid, label);
        return qid;
    }

    io_id add_cbit() {
        auto label = fmt::format("c{}", num_cbits());
        return add_cbit(label);
    }

    std::string io_label(io_id id) const { return labels_->to_label(id); }
#pragma endregion

#pragma region Properties
    std::string_view name() const { return storage_->name; }

    uint32_t gate_set() const { return storage_->gate_set; }
#pragma endregion

#pragma region Structural properties
    uint32_t size() const {
        return (storage_->nodes.size() + storage_->outputs.size());
    }

    uint32_t num_io() const { return (storage_->inputs.size()); }

    uint32_t num_qubits() const { return (storage_->num_qubits); }

    uint32_t num_cbits() const {
        return (storage_->inputs.size() - num_qubits());
    }

    uint32_t num_gates() const {
        return (storage_->nodes.size() - storage_->inputs.size());
    }
#pragma endregion

#pragma region Nodes
    node_type& get_node(node_ptr_type node_ptr) const {
        return storage_->nodes[node_ptr.index];
    }

    node_type& get_input(io_id qid) const {
        return storage_->nodes[storage_->inputs.at(qid.index())];
    }

    node_type& get_output(io_id qid) const {
        return storage_->outputs.at(qid.index());
    }

    uint32_t node_to_index(node_type const& node) const {
        if (node.gate.is(gate_lib::output)) {
            auto index = &node - storage_->outputs.data();
            return static_cast<uint32_t>(index + storage_->nodes.size());
        }
        return static_cast<uint32_t>(&node - storage_->nodes.data());
    }
#pragma endregion

#pragma region Add gates(ids)
    template <typename... Args>
    node_type& emplace_gate(Args&&... args) {
        node_type& node =
            storage_->nodes.emplace_back(std::forward<Args>(args)...);
        storage_->gate_set |=
            (1 << static_cast<uint32_t>(node.gate.operation()));
        return node;
    }

    node_type& add_gate(gate_base op, io_id target) {
        return emplace_gate(gate_type(op, storage_->rewiring_map.at(target)));
    }

    node_type& add_gate(gate_base op, io_id control, io_id target) {
        const io_id control_ = control.is_complemented()
                                   ? !storage_->rewiring_map.at(control)
                                   : storage_->rewiring_map.at(control);
        return emplace_gate(
            gate_type(op, control_, storage_->rewiring_map.at(target)));
    }

    node_type& add_gate(gate_base op, std::vector<io_id> controls,
                        std::vector<io_id> targets) {
        std::transform(controls.begin(), controls.end(), controls.begin(),
                       [&](io_id id) -> io_id {
                           const io_id real_id = storage_->rewiring_map.at(id);
                           return id.is_complemented() ? !real_id : real_id;
                       });
        std::transform(
            targets.begin(), targets.end(), targets.begin(),
            [&](io_id qid) -> io_id { return storage_->rewiring_map.at(qid); });
        return emplace_gate(gate_type(op, controls, targets));
    }
#pragma endregion

#pragma region Add gates(labels)
    node_type& add_gate(gate_base op, std::string const& qlabel_target) {
        auto qid_target = labels_->to_id(qlabel_target);
        return add_gate(op, qid_target);
    }

    node_type& add_gate(gate_base op, std::string const& qlabel_control,
                        std::string const& qlabel_target) {
        auto qid_control = labels_->to_id(qlabel_control);
        auto qid_target = labels_->to_id(qlabel_target);
        return add_gate(op, qid_control, qid_target);
    }

    node_type& add_gate(gate_base op,
                        std::vector<std::string> const& qlabels_control,
                        std::vector<std::string> const& qlabels_target) {
        std::vector<io_id> controls;
        for (auto& control : qlabels_control) {
            controls.push_back(labels_->to_id(control));
        }
        std::vector<io_id> targets;
        for (auto& target : qlabels_target) {
            targets.push_back(labels_->to_id(target));
        }
        return add_gate(op, controls, targets);
    }
#pragma endregion

#pragma region Const iterators
    template <typename Fn>
    io_id foreach_io(Fn&& fn) const {
        // clang-format off
		static_assert(std::is_invocable_r_v<void, Fn, io_id> ||
			      std::is_invocable_r_v<bool, Fn, io_id> ||
		              std::is_invocable_r_v<void, Fn, std::string const&> || 
			      std::is_invocable_r_v<void, Fn, io_id, std::string const&>);
        // clang-format on
        if constexpr (std::is_invocable_r_v<bool, Fn, io_id>) {
            for (auto const& [_, id] : *labels_) {
                if (!fn(id)) {
                    return id;
                }
            }
        } else if constexpr (std::is_invocable_r_v<void, Fn, io_id>) {
            for (auto const& [_, id] : *labels_) {
                fn(id);
            }
        } else if constexpr (std::is_invocable_r_v<void, Fn,
                                                   std::string const&>) {
            for (auto const& [label, _] : *labels_) {
                fn(label);
            }
        } else {
            for (auto const& [label, id] : *labels_) {
                fn(id, label);
            }
        }
        return io_invalid;
    }

    template <typename Fn>
    io_id foreach_qubit(Fn&& fn) const {
        // clang-format off
		static_assert(std::is_invocable_r_v<void, Fn, io_id> ||
			      std::is_invocable_r_v<bool, Fn, io_id> ||
		              std::is_invocable_r_v<void, Fn, std::string const&> || 
			      std::is_invocable_r_v<void, Fn, io_id, std::string const&>);
        // clang-format on
        if constexpr (std::is_invocable_r_v<bool, Fn, io_id>) {
            for (auto const& [_, id] : *labels_) {
                if (id.is_qubit() && !fn(id)) {
                    return id;
                }
            }
        } else if constexpr (std::is_invocable_r_v<void, Fn, io_id>) {
            for (auto const& [_, id] : *labels_) {
                if (id.is_qubit()) {
                    fn(id);
                }
            }
        } else if constexpr (std::is_invocable_r_v<void, Fn,
                                                   std::string const&>) {
            for (auto const& [label, id] : *labels_) {
                if (id.is_qubit()) {
                    fn(label);
                }
            }
        } else {
            for (auto const& [label, id] : *labels_) {
                if (id.is_qubit()) {
                    fn(id, label);
                }
            }
        }
        return io_invalid;
    }

    template <typename Fn>
    io_id foreach_cbit(Fn&& fn) const {
        // clang-format off
		static_assert(std::is_invocable_r_v<void, Fn, io_id> ||
			      std::is_invocable_r_v<bool, Fn, io_id> ||
		              std::is_invocable_r_v<void, Fn, std::string const&> || 
			      std::is_invocable_r_v<void, Fn, io_id, std::string const&>);
        // clang-format on
        if constexpr (std::is_invocable_r_v<bool, Fn, io_id>) {
            for (auto const& [_, id] : *labels_) {
                if (!id.is_qubit() && !fn(id)) {
                    return id;
                }
            }
        } else if constexpr (std::is_invocable_r_v<void, Fn, io_id>) {
            for (auto const& [_, id] : *labels_) {
                if (!id.is_qubit()) {
                    fn(id);
                }
            }
        } else if constexpr (std::is_invocable_r_v<void, Fn,
                                                   std::string const&>) {
            for (auto const& [label, id] : *labels_) {
                if (!id.is_qubit()) {
                    fn(label);
                }
            }
        } else {
            for (auto const& [label, id] : *labels_) {
                if (!id.is_qubit()) {
                    fn(id, label);
                }
            }
        }
        return io_invalid;
    }

    template <typename Fn>
    void foreach_input(Fn&& fn) const {
        // clang-format off
		static_assert(std::is_invocable_r_v<void, Fn, node_type const&, uint32_t> ||
		              std::is_invocable_r_v<void, Fn, node_type const&>);
        // clang-format on
        for (auto node_index : storage_->inputs) {
            if constexpr (std::is_invocable_r_v<void, Fn, node_type const&,
                                                uint32_t>) {
                fn(storage_->nodes[node_index], node_index);
            } else {
                fn(storage_->nodes[node_index]);
            }
        }
    }

    template <typename Fn>
    void foreach_output(Fn&& fn) const {
        // clang-format off
		static_assert(std::is_invocable_r_v<void, Fn, node_type const&, uint32_t> ||
		              std::is_invocable_r_v<void, Fn, node_type const&>);
        // clang-format on
        uint32_t node_index = storage_->nodes.size();
        for (auto const& node : storage_->outputs) {
            if constexpr (std::is_invocable_r_v<void, Fn, node_type const&,
                                                uint32_t>) {
                fn(node, node_index++);
            } else {
                fn(node);
            }
        }
    }

    template <typename Fn>
    void foreach_gate(Fn&& fn, uint32_t start = 0) const {
        foreach_element_if(
            storage_->nodes.cbegin() + start, storage_->nodes.cend(),
            [](auto const& node) { return node.gate.is_gate(); }, fn);
    }

    template <typename Fn>
    void foreach_node(Fn&& fn) const {
        foreach_element(storage_->nodes.cbegin(), storage_->nodes.cend(), fn);
        foreach_element(storage_->outputs.cbegin(), storage_->outputs.cend(),
                        fn, storage_->nodes.size());
    }
#pragma endregion

#pragma region Rewiring
    void rewire(std::vector<io_id> const& rewiring_map) {
        storage_->rewiring_map = rewiring_map;
    }

    void
    rewire(std::vector<std::pair<uint32_t, uint32_t>> const& transpositions) {
        for (auto&& [i, j] : transpositions) {
            std::swap(storage_->rewiring_map[i], storage_->rewiring_map[j]);
        }
    }

    constexpr auto rewire_map() const { return storage_->rewiring_map; }
#pragma endregion

#pragma region Visited flags
    void clear_visited() const {
        std::for_each(storage_->nodes.begin(), storage_->nodes.end(),
                      [](auto& node) { node.data[0].w = 0; });
        std::for_each(storage_->outputs.begin(), storage_->outputs.end(),
                      [](auto& node) { node.data[0].w = 0; });
    }

    auto visited(node_type const& node) const { return node.data[0].w; }

    void set_visited(node_type const& node, uint32_t value) const {
        node.data[0].w = value;
    }
#pragma endregion

  private:
    std::shared_ptr<storage_type> storage_;
    std::shared_ptr<labels_map> labels_;
};

} // namespace tweedledum
