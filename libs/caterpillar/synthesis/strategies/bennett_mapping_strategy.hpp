/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken and Giulia Meuli
*-----------------------------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <iostream>
#include <unordered_set>

#include "mapping_strategy.hpp"

#include <mockturtle/views/topo_view.hpp>

#include <fmt/format.h>

namespace caterpillar
{

namespace mt = mockturtle;

template<class LogicNetwork>
class bennett_mapping_strategy : public mapping_strategy<LogicNetwork>
{
public:
  bennett_mapping_strategy()
  {
    static_assert( mt::is_network_type_v<LogicNetwork>, "LogicNetwork is not a network type" );
    static_assert( mt::has_foreach_po_v<LogicNetwork>, "LogicNetwork does not implement the foreach_po method" );
    static_assert( mt::has_is_constant_v<LogicNetwork>, "LogicNetwork does not implement the is_constant method" );
    static_assert( mt::has_is_pi_v<LogicNetwork>, "LogicNetwork does not implement the is_pi method" );
    static_assert( mt::has_get_node_v<LogicNetwork>, "LogicNetwork does not implement the get_node method" );
  }

  virtual ~bennett_mapping_strategy() = default;

  bool compute_steps( LogicNetwork const& ntk ) override
  {
    std::unordered_set<mt::node<LogicNetwork>> drivers;
    ntk.foreach_po( [&]( auto const& f ) { drivers.insert( ntk.get_node( f ) ); } );

    auto it = this->steps().begin();
    mt::topo_view view{ntk};
    view.foreach_node( [&]( auto n ) {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
        return true;

      /* compute step */
      it = this->steps().insert( it, {n, compute_action{}} );
      ++it;

      if ( !drivers.count( n ) )
        it = this->steps().insert( it, {n, uncompute_action{}} );

      return true;
    } );

    return true;
  }
};

template<class LogicNetwork>
class bennett_inplace_mapping_strategy : public mapping_strategy<LogicNetwork>
{
public:
  bennett_inplace_mapping_strategy()
  {
    static_assert( mt::is_network_type_v<LogicNetwork>, "LogicNetwork is not a network type" );
    static_assert( mt::has_foreach_po_v<LogicNetwork>, "LogicNetwork does not implement the foreach_po method" );
    static_assert( mt::has_is_constant_v<LogicNetwork>, "LogicNetwork does not implement the is_constant method" );
    static_assert( mt::has_is_pi_v<LogicNetwork>, "LogicNetwork does not implement the is_pi method" );
    static_assert( mt::has_get_node_v<LogicNetwork>, "LogicNetwork does not implement the get_node method" );
    static_assert( mt::has_node_to_index_v<LogicNetwork>, "LogicNetwork does not implement the node_to_index method" );
    static_assert( mt::has_clear_values_v<LogicNetwork>, "LogicNetwork does not implement the clear_values method" );
    static_assert( mt::has_set_value_v<LogicNetwork>, "LogicNetwork does not implement the set_value method" );
    static_assert( mt::has_decr_value_v<LogicNetwork>, "LogicNetwork does not implement the decr_value method" );
    static_assert( mt::has_fanout_size_v<LogicNetwork>, "LogicNetwork does not implement the fanout_size method" );
    static_assert( mt::has_foreach_fanin_v<LogicNetwork>, "LogicNetwork does not implement the foreach_fanin method" );
  }

  virtual ~bennett_inplace_mapping_strategy() = default;

  bool compute_steps( LogicNetwork const& ntk ) override
  {
    std::unordered_set<mt::node<LogicNetwork>> drivers;
    ntk.foreach_po( [&]( auto const& f ) { drivers.insert( ntk.get_node( f ) ); } );

    ntk.clear_values();
    ntk.foreach_node( [&]( const auto& n ) { ntk.set_value( n, ntk.fanout_size( n ) ); } );

    auto it = this->steps().begin();
    //mt::topo_view view{ntk};
    ntk.foreach_node( [&]( auto n ) {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
        return true;

      /* decrease reference counts and mark potential target for inplace */
      int target{-1};
      ntk.foreach_fanin( n, [&]( auto f ) {
        if ( ntk.decr_value( ntk.get_node( f ) ) == 0 )
        {
          if ( target == -1 )
          {
            target = ntk.node_to_index( ntk.get_node( f ) );
          }
        }
      } );

      /* check for inplace (only if there is possible target and node is not an output driver) */
      if ( target != -1 && !drivers.count( n ) )
      {
        if constexpr ( mt::has_is_xor_v<LogicNetwork> )
        {
          if ( ntk.is_xor( n ) )
          {
            it = this->steps().insert( it, {n, compute_inplace_action{
                                                   static_cast<uint32_t>(
                                                       target ), std::nullopt}} );
            ++it;
            it = this->steps().insert( it, {n, uncompute_inplace_action{
                                                   static_cast<uint32_t>(
                                                       target ), std::nullopt}} );
            return true;
          }
        }
        if constexpr ( mt::has_is_xor3_v<LogicNetwork> )
        {
          if ( ntk.is_xor3( n ) )
          {
            it = this->steps().insert( it, {n, compute_inplace_action{
                                                   static_cast<uint32_t>(
                                                       target ), std::nullopt}} );
            ++it;
            it = this->steps().insert( it, {n, uncompute_inplace_action{
                                                   static_cast<uint32_t>(
                                                       target ), std::nullopt}} );
            return true;
          }
        }
      }

      /* compute step */
      it = this->steps().insert( it, {n, compute_action{}} );
      ++it;

      if ( !drivers.count( n ) )
        it = this->steps().insert( it, {n, uncompute_action{}} );

      return true;
    } );

    return true;
  }
};

} // namespace caterpillar