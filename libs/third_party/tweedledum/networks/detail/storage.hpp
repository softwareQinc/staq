/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "../io_id.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace tweedledum {
namespace detail {

/*! \brief
 */
template <int PointerFieldSize = 0>
struct node_pointer {
  private:
    static constexpr auto length = sizeof(uint32_t) * 8;

  public:
    static constexpr auto max = std::numeric_limits<uint32_t>::max();

    node_pointer() : data(max) {}

    node_pointer(uint32_t data) : data(data) {}

    node_pointer(uint32_t index, uint32_t weight)
        : weight(weight), index(index) {}

    union {
        uint32_t data;
        struct {
            uint32_t weight : PointerFieldSize;
            uint32_t index : length - PointerFieldSize;
        };
    };

    bool operator==(node_pointer const& other) const {
        return data == other.data;
    }
};

template <>
struct node_pointer<0> {
    static constexpr auto max = (std::numeric_limits<uint32_t>::max)();

    node_pointer() : data(max) {}

    node_pointer(uint32_t data) : data(data) {}

    union {
        uint32_t data;
        uint32_t index;
    };

    bool operator==(node_pointer const& other) const {
        return data == other.data;
    }

    bool operator!=(node_pointer const& other) const {
        return data != other.data;
    }
};

} // namespace detail

union cauint32_t {
    uint32_t w = 0;
    struct {
        uint32_t b0 : 8;
        uint32_t b1 : 8;
        uint32_t b2 : 8;
        uint32_t b3 : 8;
    };
};

/*! \brief
 */
template <typename GateType, int DataSize = 0>
struct wrapper_node {
    using pointer_type = detail::node_pointer<0>;

    GateType gate;
    mutable std::array<cauint32_t, DataSize> data;

    wrapper_node(GateType const& gate_) : gate(gate_) {}

    bool operator==(wrapper_node const& other) const {
        return gate == other.gate;
    }
};

/*! \brief
 */
template <typename GateType, int PointerFieldSize = 0, int DataSize = 0>
struct regular_node {
    using pointer_type = detail::node_pointer<PointerFieldSize>;

    GateType gate;
    std::array<std::vector<pointer_type>, GateType::max_num_io> children;
    mutable std::array<cauint32_t, DataSize> data;

    regular_node(GateType const& gate_) : gate(gate_) {}

    bool operator==(regular_node const& other) const {
        return gate == other.gate;
    }
};

/*! \brief
 */
template <typename GateType, int PointerFieldSize = 0, int DataSize = 0>
struct uniform_node {
    using pointer_type = detail::node_pointer<PointerFieldSize>;

    GateType gate;
    std::array<pointer_type, GateType::max_num_io> children;
    mutable std::array<cauint32_t, DataSize> data;

    uniform_node(GateType const& gate_) : gate(gate_) {}

    bool operator==(uniform_node const& other) const {
        return gate == other.gate;
    }
};

/*! \brief
 */
template <typename NodeType>
struct storage {
    storage(std::string_view name_ = {})
        : name(name_), num_qubits(0), gate_set(0) {
        nodes.reserve(1024u);
    }

    std::string name;
    uint32_t num_qubits;
    uint32_t gate_set;
    std::vector<uint32_t> inputs;
    std::vector<NodeType> nodes;
    std::vector<NodeType> outputs;
    std::vector<io_id> rewiring_map;
};

/*! \brief
 */
class labels_map {
  public:
    void map(io_id id, std::string const& label) {
        label_to_id_.emplace(label, id);
        id_to_label_.emplace_back(label, id);
    }

    io_id to_id(std::string const& label) const {
        return label_to_id_.at(label);
    }

    std::string to_label(io_id id) const { return id_to_label_.at(id).first; }

    auto cbegin() const { return id_to_label_.cbegin(); }

    auto cend() const { return id_to_label_.cend(); }

    auto begin() { return id_to_label_.begin(); }

    auto end() { return id_to_label_.end(); }

  private:
    std::unordered_map<std::string, io_id> label_to_id_;
    std::vector<std::pair<std::string, io_id>> id_to_label_;
};

} // namespace tweedledum
