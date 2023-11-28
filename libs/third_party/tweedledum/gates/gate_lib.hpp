/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <fmt/format.h>

namespace tweedledum {

enum class gate_lib : uint8_t {
#define GATE(X, Y, Z, V, W) X,
#include "gate_lib.def"
    num_defined_ops,
};

namespace detail {

struct table_entry {
    gate_lib adjoint;
    uint8_t rotation_axis;
    char const* symbol;
    char const* name;
};

constexpr table_entry gates_info[] = {
#define GATE(X, Y, Z, V, W) {gate_lib::Y, Z, V, W},
#include "gate_lib.def"
    {gate_lib::undefined, '-', "Err", "Error"},
};

} // namespace detail
} // namespace tweedledum
