/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken and Giulia Meuli
*-----------------------------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <fmt/format.h>

#include <mockturtle/traits.hpp>

#include "action.hpp"

namespace caterpillar {

template <class LogicNetwork>
class mapping_strategy {
  public:
    using step_function_t = std::function<void(
        mockturtle::node<LogicNetwork> const&, mapping_strategy_action const&)>;
    using step_vec_t = std::vector<
        std::pair<mockturtle::node<LogicNetwork>, mapping_strategy_action>>;

    virtual bool compute_steps(LogicNetwork const& ntk) = 0;

    void foreach_step(step_function_t const& fn) const {
        for (auto const& [n, a] : _steps) {
            fn(n, a);
        }
    }

  protected:
    step_vec_t& steps() { return _steps; }

  private:
    step_vec_t _steps;
};

template <class MappingStrategy>
void print_mapping_strategy(MappingStrategy const& strategy,
                            std::ostream& os = std::cout) {
    strategy.foreach_step([&](auto node, auto action) {
        std::visit(
            detail::overloaded{
                [](auto) {},
                [&](compute_action const&) {
                    os << fmt::format("compute({})\n", node);
                },
                [&](uncompute_action const&) {
                    os << fmt::format("uncompute({})\n", node);
                },
                [&](compute_inplace_action const& action) {
                    os << fmt::format("compute_inplace({} -> {})\n", node,
                                      action.target_index);
                },
                [&](uncompute_inplace_action const& action) {
                    os << fmt::format("uncompute_inplace({} -> {})\n", node,
                                      action.target_index);
                }},
            action);
    });
}

} // namespace caterpillar
