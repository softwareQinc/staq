/* easy: C++ ESOP library
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file maxsat.hpp
  \brief MAXSAT solver

  \author Heinz Riener
*/

#pragma once

#include <easy/sat2/sat_solver.hpp>
#include <easy/sat2/core_utils.hpp>
#include <easy/sat2/cardinality.hpp>
#include <map>

namespace easy::sat2
{

struct maxsat_linear {};
struct maxsat_uc {};
struct maxsat_rc2 {};

template<typename Algorithm>
class maxsat_solver;

class clause_to_block_vars
{
public:
  void insert( int clause_id , int var )
  {
    auto it = _map.find( clause_id );
    if ( it == _map.end() )
    {
      _map.emplace( clause_id, std::vector<int>{ var } );
    }
    else
    {
      it->second.emplace_back( var );
    }
  }

  std::vector<int> lookup( int clause_id ) const
  {
    auto const it = _map.find( clause_id );
    if ( it != _map.end() )
    {
      return it->second;
    }
    else
    {
      return {};
    }
  }

protected:
  std::unordered_map<int, std::vector<int>> _map;
}; /* clause_to_block_vars */

struct maxsat_solver_statistics
{
}; /* maxsat_solver_statistics */

struct maxsat_solver_params
{
}; /* maxsat_solver_params */

template<>
class maxsat_solver<maxsat_linear>
{
public:
  enum class state
  {
    fresh = 0,
    success = 1,
    fail = 2,
  }; /* state */

public:
  /* \brief Constructor
   *
   * Constructs a MAXSAT-solver
   *
   * \param stats Statistics
   * \param ps Parameters
   */
  explicit maxsat_solver( maxsat_solver_statistics& stats, maxsat_solver_params& ps, int& sid )
    : _stats( stats )
    , _ps( ps )
    , _sid( sid )
    , _solver( _sat_stats, _sat_params )
  {}

  /* \brief Adds a hard clause to the solver
   *
   * \param clause Clause to be added
   */
  void add_clause( std::vector<int> const& clause )
  {
    _solver.add_clause( clause );
  }

  /* \brief Adds a soft clause to the solver
   *
   * \param clause Soft clause to be added
   *
   * Returns the added activation variable.
   */
  int add_soft_clause( std::vector<int> const &clause, int weight = 1 )
  {
    auto id = _soft_clauses.size();
    _soft_clauses.emplace_back( clause );
    _weights.emplace_back( weight );
    return id;
  }

  /*
   * \brief Naive maxsat procedure based on linear search and
   *        at-most-k cardinality constraint.
   *
   * The implementation is based on Z3's MAX-SAT example [1].
   * [1] https://github.com/Z3Prover/z3/blob/master/examples/maxsat/maxsat.c
   *
   * Returns the activation variables of the soft clauses that can be
   * activated.  The return value is empty if the hard clauses cannot
   * be satisfied.
   */
  state solve()
  {
    if ( _solver.solve() == sat2::sat_solver::state::unsat )
    {
      /* it's not possible to satisfy the clauses even when ignoring all soft clauses */
      // std::cout << "[w] terminate: it's not possible to satisfy the hard clauses, even when all soft clauses are ignored" << std::endl;
      _state = state::fail;
      return _state;
    }

    /* if the number of soft-clauses is empty */
    if ( _soft_clauses.size() == 0u )
    {
      /* nothing to be done */
      // std::cout << "[w] terminate: no soft clauses" << std::endl;
      _state = state::fail;
      return _state;
    }

    /* add the soft clauses */
    std::map<int,int> selector_to_clause_id;
    for ( auto i = 0; i < _soft_clauses.size(); ++i )
    {
      int selector = _sid++;
      selector_to_clause_id.emplace( selector, i );
      _selectors.push_back( -selector );

      auto& cl = _soft_clauses[i];
      cl.emplace_back( -selector );
      _solver.add_clause( cl );
      _disabled_clauses.push_back( i );
    }

    std::vector<std::vector<int>> clauses;
    auto at_most_k = create_totalizer( clauses, _sid, _selectors, _selectors.size() );
    for ( const auto& c : clauses )
    {
      add_clause( c );
    }

    /* enforce that at most k soft clauses are satisfied */
    uint32_t k = _selectors.size() - 1u;

    /* perform linear search */
    for ( ;; )
    {
      // std::cout << "[i] try with k = " << k << std::endl;

      /* disable at-most k selectors */
      std::vector<int> assumptions;
      for ( auto i = 0; i < at_most_k->vars.size(); ++i )
      {
        assumptions.emplace_back( i < k ? at_most_k->vars[i] : -at_most_k->vars[i] );
      }

      if ( _solver.solve( assumptions ) == sat2::sat_solver::state::unsat )
      {
        /* unsat */
        _state = state::success;
        return _state;
      }

      auto const m = _solver.get_model();

      _enabled_clauses.clear();
      _disabled_clauses.clear();
      for ( const auto& s : _selectors )
      {
        if ( m[s] )
        {
          _disabled_clauses.push_back( selector_to_clause_id.at( -s ) );
        }
        else if ( !m[s] )
        {
          _enabled_clauses.push_back( selector_to_clause_id.at( -s ) );
        }
      }

      k = std::count_if( _selectors.begin(), _selectors.end(),
                         [&m]( auto const& s ){ return m[s]; });
      if ( k == 0 )
      {
        _state = state::fail;
        return _state;
      }
      --k;
    }
  }

  std::vector<int> get_enabled_clauses() const
  {
    return _enabled_clauses;
  }

  std::vector<int> get_disabled_clauses() const
  {
    return _disabled_clauses;
  }

protected:
  state _state = state::fresh;

  maxsat_solver_statistics& _stats;
  maxsat_solver_params const& _ps;
  int& _sid;

  sat_solver_statistics _sat_stats;
  sat_solver_params _sat_params;
  sat_solver _solver;

  std::vector<int> _selectors;

  std::vector<int> _enabled_clauses;
  std::vector<int> _disabled_clauses;

  std::vector<std::vector<int>> _soft_clauses;
  std::vector<int> _weights;
}; /* maxsat_solver<maxsat_linear> */

template<>
class maxsat_solver<maxsat_uc>
{
public:
  enum class state
  {
    fresh = 0,
    success = 1,
    fail = 2,
  }; /* state */

public:
  /* \brief Constructor
   *
   * Constructs a MAXSAT-solver
   *
   * \param stats Statistics
   * \param ps Parameters
   */
  explicit maxsat_solver( maxsat_solver_statistics& stats, maxsat_solver_params& ps, int& sid )
    : _stats( stats )
    , _ps( ps )
    , _sid( sid )
    , _solver( _sat_stats, _sat_params )
  {}

  /* \brief Adds a hard clause to the solver
   *
   * \param clause Clause to be added
   */
  void add_clause( std::vector<int> const& clause )
  {
    _solver.add_clause( clause );
  }

  /* \brief Adds a soft clause to the solver
   *
   * \param clause Soft clause to be added
   *
   * Returns the added activation variable.
   */
  int add_soft_clause( std::vector<int> const &clause, int weight = 1 )
  {
    auto id = _soft_clauses.size();
    _soft_clauses.emplace_back( clause );
    _weights.emplace_back( weight );
    return id;
  }

  void add_one_hot_clauses( std::vector<int> lits )
  {
    if ( lits.size() == 1 )
    {
      add_clause( lits );
      return;
    }

    std::vector<std::vector<int>> clauses;
    auto at_most_1 = create_totalizer( clauses, _sid, lits, 2u );
    for ( const auto& c : clauses )
    {
      add_clause( c );
    }

    /* enable at-most 1 lits */
    add_clause( { at_most_1->vars[0u] } );
    for ( auto i = 1; i < at_most_1->vars.size(); ++i )
    {
      add_clause( { -at_most_1->vars[i] } );
    }

    add_clause( lits );
  }

  /*
   * \brief Fu&Malik MAXSAT procedure using UNSAT core extraction and
   * the at-most-k cardinality constraint.
   *
   * The implementation is based on Z3's MAX-SAT example [1].  As
   * seminal reference of the algorithm serves [2].
   *
   * [1] https://github.com/Z3Prover/z3/blob/master/examples/maxsat/maxsat.c
   * [2] Zhaohui Fu, Sharad Malik: On Solving the Partial MAX-SAT
   *    Problem. SAT 2006: 252-265
   */
  state solve()
  {
    if ( _solver.solve() == sat2::sat_solver::state::unsat )
    {
      /* it's not possible to satisfy the clauses even when ignoring all soft clauses */
      // std::cout << "[w] terminate: it's not possible to satisfy the hard clauses, even when all soft clauses are ignored" << std::endl;
      _state = state::fail;
      return _state;
    }

    /* if the number of soft-clauses is empty */
    if ( _soft_clauses.size() == 0u )
    {
      /* nothing to be done */
      // std::cout << "[w] terminate: no soft clauses" << std::endl;
      _state = state::fail;
      return _state;
    }

    /* add the soft clauses */
    std::map<int,int> selector_to_clause_id;
    for ( auto i = 0; i < _soft_clauses.size(); ++i )
    {
      int selector = _sid++;
      selector_to_clause_id.emplace( selector, i );
      _selectors.push_back( -selector );

      auto cl = _soft_clauses[i];
      cl.emplace_back( -selector );
      add_clause( cl );
    }

    clause_to_block_vars block_variables;

    auto iteration = 0;
    for ( ;; )
    {
      /* assume all soft-clauses are enabled, which causes the problem to UNSAT */
      std::vector<int> assumptions;
      for ( const auto& s : _selectors )
      {
        assumptions.emplace_back( -s );
      }

      auto const state = _solver.solve( assumptions );
      if ( state == sat2::sat_solver::state::sat )
      {
        auto m = _solver.get_model();

        _disabled_clauses.clear();
        _enabled_clauses.clear();

        for ( auto i = 0; i < _selectors.size(); ++i )
        {
          if ( m[ -_selectors[i] ] )
          {
            /* evaluate block variables of the i-th clause */
            auto is_blocked = false;
            for ( const auto& v : block_variables.lookup( i ) )
            {
              if ( m[ v ] )
              {
                is_blocked = true;
                break;
              }
            }

            if ( !is_blocked )
            {
              _enabled_clauses.push_back( i );
              continue;
            }
          }

          /* otherwise, the clause is enabled */
          _disabled_clauses.push_back( i );
        }

        _state = state::success;
        return _state;
      }
      else
      {
        auto const core = _solver.get_core();

        std::vector<int> block_vars( core.size() );
        for ( auto i = 0; i < core.size(); ++i )
        {
          int sel = _sid++;
          int b = _sid++;

          auto const j = selector_to_clause_id.at( core[i] );
          auto& cl = _soft_clauses[j];
          cl.emplace_back( b );

          _selectors[j] = -sel;
          selector_to_clause_id.emplace( sel, j );

          block_vars[i] = b;
          block_variables.insert( j, b );

          std::vector<int> cl2( cl );
          cl2.emplace_back( -sel );
          add_clause( cl2 );
        }

        /* ensure that at-most-1 block vars is one */
        add_one_hot_clauses( block_vars );
      }

      ++iteration;
    }
  }

  std::vector<int> get_enabled_clauses() const
  {
    return _enabled_clauses;
  }

  std::vector<int> get_disabled_clauses() const
  {
    return _disabled_clauses;
  }

protected:
  state _state = state::fresh;

  maxsat_solver_statistics& _stats;
  maxsat_solver_params const& _ps;
  int& _sid;

  sat_solver_statistics _sat_stats;
  sat_solver_params _sat_params;
  sat_solver _solver;

  std::vector<int> _selectors;

  std::vector<int> _enabled_clauses;
  std::vector<int> _disabled_clauses;

  std::vector<std::vector<int>> _soft_clauses;
  std::vector<int> _weights;
}; /* maxsat_solver<maxsat_uc> */

template<>
class maxsat_solver<maxsat_rc2>
{
public:
  enum class state
  {
    fresh = 0,
    success = 1,
    fail = 2,
  }; /* state */

public:
  /* \brief Constructor
   *
   * Constructs a MAXSAT-solver
   *
   * \param stats Statistics
   * \param ps Parameters
   */
  explicit maxsat_solver( maxsat_solver_statistics& stats, maxsat_solver_params& ps, int& sid )
    : _stats( stats )
    , _ps( ps )
    , _sid( sid )
    , _solver( _sat_stats, _sat_params )
  {}

  /* \brief Adds a hard clause to the solver
   *
   * \param clause Clause to be added
   */
  void add_clause( std::vector<int> const& clause )
  {
    _solver.add_clause( clause );
  }

  /* \brief Adds a soft clause to the solver
   *
   * \param clause Soft clause to be added
   *
   * Returns the added activation variable.
   */
  int add_soft_clause( std::vector<int> const &clause, int weight = 1 )
  {
    auto id = _soft_clauses.size();
    _soft_clauses.emplace_back( clause );
    _weights.emplace_back( weight );
    return id;
  }

  /*
   * \brief RC2 MAXSAT procedure
   *
   * The implementation is based on pysat's RC2 example [1].
   *
   * [1] https://github.com/pysathq/pysat/blob/master/examples/rc2.py
   */
  state solve()
  {
    if ( _solver.solve() == sat2::sat_solver::state::unsat )
    {
      /* it's not possible to satisfy the clauses even when ignoring all soft clauses */
      // std::cout << "[w] terminate: it's not possible to satisfy the hard clauses, even when all soft clauses are ignored" << std::endl;
      _state = state::fail;
      return _state;
    }

    /* if the number of soft-clauses is empty */
    if ( _soft_clauses.size() == 0u )
    {
      /* nothing to be done */
      // std::cout << "[w] terminate: no soft clauses" << std::endl;
      _state = state::fail;
      return _state;
    }

    std::vector<int> sels;
    std::vector<int> sums;
    std::map<int,std::shared_ptr<totalizer_tree>> t_objects;
    std::map<int,int> bounds;
    std::map<int, int> selector_to_clause;
    int costs = 0;

    /* add the soft clauses */
    for ( auto i = 0; i < _soft_clauses.size(); ++i )
    {
      auto cl = _soft_clauses[i];

      /* if clause is unit, selector variable is its literal */
      int selector = cl[0u];

      if ( _soft_clauses[i].size() > 1 )
      {
        selector = _sid++;
        cl.push_back( -selector );
        add_clause( cl );
      }

      sels.push_back( selector );
      selector_to_clause.emplace( selector, i );
    }

    /* make a copy of the selectors */
    _selectors = sels;

    auto iteration = 0;
    for ( ;; )
    {
      /* assume all soft-clauses are enabled, which causes the problem to be UNSAT */
      std::vector<int> assumptions;
      for ( const auto& s : sels )
      {
        assumptions.emplace_back( s );
      }
      for ( const auto& s : sums )
      {
        assumptions.emplace_back( s );
      }

      auto const state = _solver.solve( assumptions );
      if ( state == sat2::sat_solver::state::sat )
      {
        auto const model = _solver.get_model();
        for ( auto i = 0; i < _soft_clauses.size(); ++i )
        {
          if ( model[_selectors[i]] )
          {
            _enabled_clauses.push_back( i );
          }
          else
          {
            _disabled_clauses.push_back( i );
          }
        }
        // assert( _disabled_clauses.size() <= costs );
        return state::success;
      }

      auto const core = _solver.get_core();
      // std::cout << "[i] core: "; core.print(); std::cout << std::endl;

      /* divide core into sels and sums */
      std::vector<int> core_sels;
      std::vector<int> core_sums;
      for ( auto i = 0; i < core.size(); ++i )
      {
        if ( std::find( std::begin( sels ), std::end( sels ), core[i] ) != std::end( sels ) )
        {
          core_sels.push_back( core[i] );
          continue;
        }

        if ( std::find( std::begin( sums ), std::end( sums ), core[i] ) != std::end( sums ) )
        {
          core_sums.push_back( core[i] );
          continue;
        }
      }

      /* update costs */
      int w_min = std::numeric_limits<int>::max();
      for ( auto i = 0; i < core.size(); ++i )
      {
        auto const clause_id = selector_to_clause.at( core[i] );
        auto const w = _weights.at( clause_id );
        if ( w < w_min )
          w_min = w;
      }

      // std::cout << "[i] w_min = " << w_min << std::endl;

      /* process core */
      costs += w_min;
      std::vector<int> garbage;
      if ( core_sels.size() != 1 || core_sums.size() > 0 )
      {
        /* processs sels */
        std::vector<int> rels;
        for ( const auto& l : core_sels )
        {
          auto const clause_id = selector_to_clause.at( l );
          auto& w = _weights.at( clause_id );
          if ( w == w_min )
          {
            /* marking variables as being a part of the core, so that
               next time it is not used as an assumption */
            garbage.push_back( l );

            /* reuse assumption variables as relaxation */
            rels.push_back( -l );
          }
          else
          {
            /* do not remove this variable from assumptions since it
               has a remaining non-zero weight */
            w -= w_min;

            /* it is an unrelaxed soft-clause, a new relaxed copy of
               which we add to the solver */
            auto sid = _sid++;
            add_clause( { l, sid } );
            rels.push_back( sid );
          }
        }

        /* process sums */
        for ( const auto& l : core_sums )
        {
          auto const clause_id = selector_to_clause.at( l );
          auto& w = _weights[clause_id];
          if ( w == w_min )
          {
            /* marking variable as being a part of the core so that
               next time it is not used as an assumption */
            garbage.push_back( l );
          }
          else
          {
            /* do not remove this variable from assumptions since it
               has a remaining non-zero weight */
            w -= w_min;
          }

          /* increase bound for the sum */
          auto& t = t_objects[l];
          auto b = bounds[l]+1;

          std::vector<std::vector<int>> clauses;
          increase_totalizer( clauses, _sid, t, b );
          for ( const auto& c : clauses )
          {
            add_clause( c );
          }

          /* updating bounds and weights */
          if ( b < t->vars.size() )
          {
            auto lnew = -t->vars[b];
            if ( std::find( std::begin( garbage ), std::end( garbage ), lnew ) != std::end( garbage ) )
            {
              garbage.erase( std::remove( std::begin( garbage ), std::end( garbage ), lnew ) );
              _weights[selector_to_clause[lnew]] = 0;
            }

            if ( selector_to_clause.find( lnew ) == selector_to_clause.end() )
            {
              /* invoke set bounds */
              t_objects.emplace( -t->vars[b], t );
              bounds.emplace( -t->vars[b], b );
              selector_to_clause.emplace( -t->vars[b], _weights.size() );
              _weights.push_back( w_min );

              sums.push_back( -t->vars[b] );
            }
            else
            {
              _weights[selector_to_clause[lnew]] += w_min;
            }
          }

          rels.push_back( -l );
        }

        if ( rels.size() > 1 )
        {
          /* create a new cardiality constraint */
          std::vector<std::vector<int>> clauses;
          auto totalizer_tree = create_totalizer( clauses, _sid, rels, 1u );
          for ( const auto& c : clauses )
          {
            add_clause( c );
          }

          /* TODO: exhaust core */
          auto b = 1;
          /* invoke set bound */

          /* save the info about this sum and add its assumption literal */
          t_objects.emplace( -totalizer_tree->vars[b], totalizer_tree );
          bounds.emplace( -totalizer_tree->vars[b], b );
          selector_to_clause.emplace( -totalizer_tree->vars[b], _weights.size() );
          _weights.push_back( w_min );

          sums.push_back( -totalizer_tree->vars[b] );
        }
      }
      else
      {
        assert( core_sels.size() == 1u );

        /* special case: unit core */
        add_clause( { -core_sels[0] } );
        garbage.push_back( core_sels[0] );
      }

      /* cleanup garbage */
      sels.erase( std::remove_if( std::begin( sels ), std::end( sels ),
        [&garbage]( auto const &x ){
          return std::find( std::begin( garbage ), std::end( garbage ), x) != std::end( garbage );
        }), std::end( sels ) );
      sums.erase( std::remove_if( std::begin( sums ), std::end( sums ),
        [&garbage]( auto const &x ){
          return std::find( std::begin( garbage ), std::end( garbage ), x) != std::end( garbage );
        }), std::end( sums ) );
      garbage.clear();

      // std::cout << "c cost: " << costs << ' ';
      // std::cout << "core sz: " << core.size() << ' ';
      // std::cout << "soft sz: " << ( sums.size() + sels.size() ) << ' ';
      // std::cout << std::endl;

      ++iteration;
    }

    return state::fail;
  }

  std::vector<int> get_enabled_clauses() const
  {
    return _enabled_clauses;
  }

  std::vector<int> get_disabled_clauses() const
  {
    return _disabled_clauses;
  }

protected:
  state _state = state::fresh;

  maxsat_solver_statistics& _stats;
  maxsat_solver_params const& _ps;
  int& _sid;

  sat_solver_statistics _sat_stats;
  sat_solver_params _sat_params;
  sat_solver _solver;

  std::vector<int> _selectors;

  std::vector<int> _enabled_clauses;
  std::vector<int> _disabled_clauses;

  std::vector<std::vector<int>> _soft_clauses;
  std::vector<int> _weights;
}; /* maxsat_solver<maxsat_rc2> */

} /* easy::sat2 */
