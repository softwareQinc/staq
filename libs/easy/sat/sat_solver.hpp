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

#pragma once

#include <easy/sat/constraints.hpp>
#include <cassert>
#include <memory>
#include <vector>

namespace easy::sat
{

struct sat_solver
{
  using assumptions_t = std::vector<int>;
  using state_t = Glucose::lbool;
  using model_t = std::vector<Glucose::lbool>;

  struct result
  {
    result( state_t state = Glucose::l_Undef )
        : state( state )
    {
    }

    result( const model_t& m )
        : state( Glucose::l_True ), model( m )
    {
    }

    inline operator bool() const { return ( state == Glucose::l_True ); }

    inline bool is_sat() const { return ( state == Glucose::l_True ); }
    inline bool is_unsat() const { return ( state == Glucose::l_False ); }
    inline bool is_undef() const { return ( state == Glucose::l_Undef ); }

    state_t state;
    model_t model;
  }; /* result */

  sat_solver();
  result solve( constraints& constraints, const assumptions_t& assumptions = {} );
  void reset();

  void set_conflict_limit( int limit );
  int get_conflicts() const;

  unsigned _num_vars = 0;

  /* -1 indicates no conflict limit */
  int _conflict_limit = -1;

  std::unique_ptr<Glucose::Solver> _solver;
};

inline sat_solver::sat_solver()
{
  _solver = std::make_unique<Glucose::Solver>();
}

inline void sat_solver::reset()
{
  _solver = std::make_unique<Glucose::Solver>();
  _num_vars = 0;
  _solver->budgetOff();
}

inline void sat_solver::set_conflict_limit( int limit )
{
  _conflict_limit = limit;
  _solver->setConfBudget( limit );
}

inline int sat_solver::get_conflicts() const
{
  return _solver->conflicts;
}

inline sat_solver::result sat_solver::solve( constraints& constraints, const assumptions_t& assumptions )
{
  /* add clauses to solver & remove them from constraints */
  constraints.foreach_clause( [&]( constraints::clause_t const& c ){
      Glucose::vec<Glucose::Lit> clause;
      for ( const auto& l : c )
      {
        const unsigned var = abs( l ) - 1;
        while ( _num_vars <= var )
        {
          _solver->newVar();
          ++_num_vars;
        }
        clause.push( Glucose::mkLit( var, l < 0 ) );
      }
      _solver->addClause( clause );
    });
  constraints.clear_clauses();

  /* add xor clauses to solver & remove them from constraints */
  assert( constraints.num_xor_clauses() == 0u );

  bool sat;

  Glucose::vec<Glucose::Lit> assume;
  if ( assumptions.size() > 0 )
  {
    for ( const auto& v : assumptions )
    {
      const unsigned var = abs( v ) - 1;
      while ( _num_vars <= var )
      {
        _solver->newVar();
        ++_num_vars;
      }
      assume.push( Glucose::mkLit( var, v < 0 ) );
    }
  }

  if ( _conflict_limit == -1 )
  {
    sat = _solver->solve( assume );
  }
  else
  {
    const auto solver_result = _solver->solveLimited( assume );
    if ( solver_result == Glucose::l_Undef || int32_t(_solver->conflicts) >= _conflict_limit )
    {
      return result( Glucose::l_Undef );
    }
    else
    {
      assert( solver_result == Glucose::l_True || solver_result == Glucose::l_False );
      sat = solver_result == Glucose::l_True;
    }
  }

  if ( sat )
  {
    std::vector<Glucose::lbool> model( _num_vars );
    for ( auto i = 0u; i < _num_vars; ++i )
    {
      model[i] = _solver->modelValue( i );
    }
    return result( model );
  }
  else
  {
    return result( sat ? Glucose::l_True : Glucose::l_False );
  }
}

} // namespace easy::sat

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
