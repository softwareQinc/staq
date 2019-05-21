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

#include <easy/esop/esop.hpp>

namespace easy::esop
{

/*! \brief Compute the T-count of a product term
 *
 * Compute the T-count of a product term.
 *
 * \param cube A product term
 * \param num_vars Total number of lines (or qubits)
 * \return T-count, i.e., the number of T-gates for realizing the product term
 */
inline uint64_t T_count( kitty::cube const& cube, uint32_t num_vars )
{
  auto const ac = cube.num_literals();

  switch( ac )
  {
  case 0u:
  case 1u:
    return 0u;
  case 2u:
    return 7u;
  default:
    break;
  }

  if ( (num_vars - ac - 1) >= (ac - 1)/2 )
  {
    return 8 * ( ac - 1u );
  }
  else
  {
    return 16 * ( ac -1u );
  }
}

/*! \brief Compute the T-count of an ESOP form
 *
 * Compute the T-count of an ESOP form.
 *
 * \param cube An ESOP form
 * \param num_vars Total number of lines (or qubits)
 * \return T-count, i.e., the number of T-gates for realizing the ESOP form
 */
inline uint64_t T_count( esop_t const& esop, uint32_t num_vars )
{
  uint64_t total_cost = 0u;
  for ( auto i = 0u; i < esop.size(); ++i )
  {
    total_cost += T_count( esop[i], num_vars );
  }
  return total_cost;
}
  
} // namespace easy::esop

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:

