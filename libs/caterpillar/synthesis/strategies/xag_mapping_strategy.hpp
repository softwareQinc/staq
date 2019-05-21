/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli
*------------------------------------------------------------------------------------------------*/

#pragma once

#include "mapping_strategy.hpp"
#include <caterpillar/stg_gate.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <tweedledum/networks/netlist.hpp>

#include <algorithm>


namespace caterpillar
{

struct action_sets
{
  mockturtle::node<mockturtle::xag_network> node;
  std::vector<uint32_t> target;
  std::vector<uint32_t> leaves;

  action_sets( mockturtle::node<mockturtle::xag_network> node, std::vector<uint32_t> leaves, std::vector<uint32_t> target = {} )
      : node(node), target(target), leaves(leaves) {};
};

class xag_mapping_strategy : public mapping_strategy<mockturtle::xag_network>
{
  std::vector<uint32_t> sym_diff(std::vector<uint32_t> first, std::vector<uint32_t> second)
  {
    std::vector<uint32_t> diff;
    std::set_symmetric_difference( first.begin(), first.end(), second.begin(), second.end(), std::back_inserter(diff) );
    return diff;
  }

  void compute_fi( mockturtle::node<mockturtle::xag_network> node)
  {
    if ( xag.is_and( node ) || xag.is_pi(node))
    {
      fi[ xag.node_to_index(node) ] = { xag.node_to_index(node) };
    }

    else
    {      
      std::vector<uint32_t> fanin;
      xag.foreach_fanin(node, [&]( auto si ) {
        fanin.push_back(xag.node_to_index(xag.get_node(si)));
      } );

      
      fi[xag.node_to_index( node )] = sym_diff( fi[fanin[0]], fi[fanin[1]] );
    }
    
  }

  std::vector<action_sets> get_fi_target( mockturtle::node<mockturtle::xag_network> node )
  {
    std::vector<action_sets> chs;

    std::vector<mockturtle::node<mockturtle::xag_network>> fanins;
    xag.foreach_fanin( node, [&]( auto si ) {
      auto fanin = xag.get_node( si );
      if (xag.is_xor(fanin))
      {
        auto set = action_sets(fanin, fi[xag.node_to_index( fanin )] ) ; 
        chs.push_back( set ); 
      }     
      fanins.push_back(fanin);
    } );

    if(chs.size() == 2)
    {
      if ( first_cone_included_in_second(fi[xag.node_to_index(chs[1].node)], fi[xag.node_to_index(chs[0].node)]) )
        std::reverse(chs.begin(), chs.end());

      /* search a target for first */
      /* empty if first is included */
      std::set_difference(fi[xag.node_to_index(chs[0].node)].begin(), fi[xag.node_to_index(chs[0].node)].end(), 
        fi[xag.node_to_index( chs[1].node )].begin(), fi[xag.node_to_index( chs[1].node )].end(), std::inserter(chs[0].target, chs[0].target.begin()));

      /* set difference */
      std::set_difference(fi[xag.node_to_index(chs[1].node)].begin(), fi[xag.node_to_index(chs[1].node)].end(), 
        fi[xag.node_to_index( chs[0].node )].begin(), fi[xag.node_to_index( chs[0].node )].end(), std::inserter(chs[1].target, chs[1].target.begin()));
      
      /* the first may be included */
      if( first_cone_included_in_second(fi[xag.node_to_index(chs[0].node)], fi[xag.node_to_index(chs[1].node)] ) )
      {
        /* i must add the top of the cone to the second */
        chs[1].target.push_back( chs[0].node );
        chs[1].leaves = chs[1].target;

        /* anything can be chosen as target */ 
        chs[0].target = fi[xag.node_to_index( chs[0].node )];
      }

    }

    if(chs.size() == 1)
    {
      if ( xag.is_xor( fanins[1] ) )
        std::reverse(fanins.begin(), fanins.end());

      std::set_difference(fi[xag.node_to_index(fanins[0])].begin(), fi[xag.node_to_index(fanins[0])].end(), 
        fi[xag.node_to_index(fanins[1])].begin(), fi[xag.node_to_index(fanins[1])].end(), std::inserter(chs[0].target, chs[0].target.begin()) );

    }

    return chs;
  }

  bool first_cone_included_in_second(std::vector<uint32_t> first, std::vector<uint32_t> second)
  {
    /* if second is included in first */
    bool is_included = true;
    for(auto l : first)
    {
      if (std::find( second.begin(), second.end(), l) == second.end())
      {
        is_included = false;
        break;
      }
    }
    return is_included;
  }


  std::vector<std::pair<mockturtle::node<mockturtle::xag_network>, mapping_strategy_action>> compute( mockturtle::node<mockturtle::xag_network> node, bool compute)
  {
    std::vector<std::pair<mockturtle::node<mockturtle::xag_network>, mapping_strategy_action>> comp_steps;
    
    auto chs = get_fi_target(  node );

    for(auto ch : chs)
    {
      if ( !ch.target.empty() )
      {
        comp_steps.push_back( {ch.node, compute_inplace_action{static_cast<uint32_t>( ch.target[0] ), ch.leaves}} );
      }
      else 
      {
        comp_steps.push_back( {ch.node, compute_action{ ch.leaves, std::nullopt }} );
      }
    }
    
    if(xag.is_and(node))
    {
      if(compute)
        comp_steps.push_back( {node, compute_action{}} );
      else 
      {
        comp_steps.push_back( {node, uncompute_action{}} );
      }
    }
    else 
    {
      comp_steps.push_back( {node, compute_action{}} );
    }

    std::reverse( chs.begin(), chs.end() );

    for(auto ch : chs)
    {
      if ( !ch.target.empty() )
      {
        comp_steps.push_back( {ch.node, compute_inplace_action{static_cast<uint32_t>( ch.target[0] ), ch.leaves}} );
      }
      else 
      {
        comp_steps.push_back( {ch.node, compute_action{ ch.leaves, std::nullopt}} );
      }
    }

    return comp_steps;
  }

  std::vector<std::vector<uint32_t>> fi;
  mockturtle::xag_network xag;

public:
  bool compute_steps( mockturtle::xag_network const& ntk ) override
  {
    mockturtle::topo_view topo {ntk};
    xag = topo;

    std::vector<mockturtle::node<mockturtle::xag_network>> drivers;
    xag.foreach_po( [&]( auto const& f ) { drivers.push_back( xag.get_node( f ) ); } );
                                                                                   

    auto it = steps().begin();

    fi.resize( xag.size() );


    xag.foreach_node( [&]( auto node ) {

      compute_fi( node );

      if ( xag.is_and( node ) )
      {

        
        /* compute step */
        auto cc = compute( node , true);
        it = steps().insert( it, cc.begin(), cc.end() );
        it = it + cc.size();

        if ( std::find( drivers.begin(), drivers.end(), node ) == drivers.end() )
        {
          auto uc = compute( node , false);
          it = steps().insert( it, uc.begin(), uc.end() );
        }
      }
      /* node is an XOR output */
      else if ( std::find( drivers.begin(), drivers.end(), node ) != drivers.end() )
      {
        auto xc = compute( node, true);
        it = steps().insert( it, xc.begin(), xc.end() );

      }

    } );

    return true;
  }
};

} // namespace caterpillar