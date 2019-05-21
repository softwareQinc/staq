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
  \file core_utils.hpp
  \brief Utility functions for UNSAT cores.

  \author Heinz Riener

  The UNSAT core optimization heuristics are based on the
  implementation of RC2 by Alexey S. Ignatiev [1].

  [1] https://github.com/pysathq/pysat/blob/master/examples/rc2.py
*/

#pragma once

#include <easy/sat2/sat_solver.hpp>

namespace easy::sat2
{

namespace detail
{

template<typename T>
inline std::vector<T> copy_vector_without_index( std::vector<T> const& vs, uint64_t index )
{
  assert( index < vs.size() );
  std::vector<T> copy( vs );
  copy.erase( std::begin( copy ) + index );
  return copy;
}

} /* namespace detail */

/* \brief Trim unsatisfiable core
 *
 * Given a SAT-solver `solver` with an unsatisfiable core `cs`, such
 * that the solver is in state UNSAT under the given assumptions.  The algorithm
 * heuristically tries to trim `cs`.  The core is trimmed at
 * most `num_tries`-times.
 *
 * \param solver SAT-solver
 * \param cs An unsatisfiable core
 * \param num_tries Maximal number of tries to trim core
 *
 * Returns the trimmed core.
 */
inline core<> trim_core_copy( sat_solver& solver, core<> const& cs, uint32_t num_tries = 8u )
{
  auto current = cs;

  uint32_t counter = 0;
  while ( counter++ < num_tries && solver.solve( current ) == sat_solver::state::unsat )
  {
    auto const new_core = solver.get_core();
    if ( new_core.size() == current.size() )
    {
      break;
    }
    else
    {
      current = new_core;
    }
  }

  return current;
}

/* \brief Trim unsatisfiable core
 *
 * Given a SAT-solver `solver` with an unsatisfiable core `cs`, such
 * that the solver is in state UNSAT under the given assumptions.  The algorithm
 * heuristically tries to trim `cs` inplace.  The core is trimmed at
 * most `num_tries`-times.
 *
 * \param solver SAT-solver
 * \param cs An unsatisfiable core
 * \param num_tries Maximal number of tries to trim core
 */
inline void trim_core( sat_solver& solver, core<>& cs, uint32_t num_tries = 8u )
{
  cs = trim_core_copy( solver, cs, num_tries );
}

/* \brief Deletion-based unsatisfiable core minimization
 *
 * Given a SAT-solver `solver` with an unsatisfiable core `cs`, such
 * that the solver is in state UNSAT undert the given assumptions, the
 * algorithm heuristically tries to minimize the core using an
 * `budget` as conflict limit.
 *
 * \param solver SAT-solver
 * \param cs An unsatisfiable core
 * \param budget A budget limit for SAT-solving
 *
 * Returns a potentially minimized unsatisfiable core.
 */
inline core<> minimize_core_copy( sat_solver& solver, core<> const& cs, int64_t budget = 1000 )
{
  solver.set_budget( budget );

  auto pos = 0u;
  auto current = cs;

  while ( pos < current.size() )
  {
    auto temp = core<>( detail::copy_vector_without_index( std::vector<int>( current ), pos ) );
    solver.solve_limited( temp );

    if ( solver.is_unsat() )
    {
      current = temp;
    }
    else
    {
      ++pos;
    }
  }

  solver.reset_budget();

  if ( current.size() < cs.size() )
  {
    return current;
  }
  else
  {
    return cs;
  }
}

/* \brief Deletion-based unsatisfiable core minimization
 *
 * Given a SAT-solver `solver` with an unsatisfiable core `cs`, such
 * that the solver is in state UNSAT undert the given assumptions, the
 * algorithm heuristically tries to minimize the core using an
 * `budget` as conflict limit.
 *
 * \param solver SAT-solver
 * \param cs An unsatisfiable core
 * \param budget A budget limit for SAT-solving
 *
 */
inline void minimize_core( sat_solver& solver, core<>& cs, int64_t budget = 1000 )
{
  cs = minimize_core_copy( solver, cs, budget );
}

} /* easy::sat2 */
