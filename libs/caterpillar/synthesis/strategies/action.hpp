/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*-----------------------------------------------------------------------------*/
#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>

namespace caterpillar
{

struct compute_action
{
  std::optional<std::vector<uint32_t>> leaves;
  /*! \brief Override node as cell.
   *
   * If a value is assigned to this variable, instead of using the gate
   * associated to the computed node, the cell with respective truth table
   * and leaves is considered.
   */
  std::optional<std::pair<kitty::dynamic_truth_table, std::vector<uint32_t>>> cell_override;

  
};

struct uncompute_action
{
  std::optional<std::vector<uint32_t>> leaves;
  /*! \brief Override node as cell.
   *
   * If a value is assigned to this variable, instead of using the gate
   * associated to the uncomputed node, the cell with respective truth table
   * and leaves is considered.
   */
  std::optional<std::pair<kitty::dynamic_truth_table, std::vector<uint32_t>>> cell_override;

};

struct compute_inplace_action
{
  uint32_t target_index;
  std::optional<std::vector<uint32_t>> leaves;
};

struct uncompute_inplace_action
{
  uint32_t target_index;
  std::optional<std::vector<uint32_t>> leaves;
};

using mapping_strategy_action = std::variant<compute_action, uncompute_action, compute_inplace_action, uncompute_inplace_action>;

namespace detail
{
template<class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template<class... Ts>
overloaded( Ts... )->overloaded<Ts...>;
} // namespace detail

}
