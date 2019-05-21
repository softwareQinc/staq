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
  \file constructors.hpp
  \brief Implements constructors for exclusive-or sum-of-product forms.

  \author Mathias Soeken
  \author Winston Haaswijk
*/

#pragma once

#include <easy/esop/esop.hpp>
#include <easy/esop/cube_manipulators.hpp>
#include <kitty/cube.hpp>

#include <unordered_set>

namespace easy
{

namespace esop
{

/*! \cond PRIVATE */
namespace detail
{

template<typename TT>
inline void esop_from_pprm_rec( std::unordered_set<kitty::cube, kitty::hash<kitty::cube>>& cubes, const TT& tt, uint8_t var_index, const kitty::cube& c )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return;
  }
  if ( is_const0( ~tt ) )
  {
    /* add to cubes, but do not apply distance-1 merging */
    add_to_cubes( cubes, c, false );
    return;
  }

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  esop_from_pprm_rec( cubes, tt0, var_index + 1, c );
  esop_from_pprm_rec( cubes, tt0 ^ tt1, var_index + 1, with_literal( c, var_index, true ) );
}

} // namespace detail
/*! \endcond */

/*! \brief Computes PPRM representation for a function

  This algorithm applies recursively the positive Davio decomposition which
  eventually leads into the PPRM representation of a function.

  \param tt Truth table
*/
template<typename TT>
inline esop_t esop_from_pprm( const TT& tt )
{
  std::unordered_set<kitty::cube, kitty::hash<kitty::cube>> cubes;
  detail::esop_from_pprm_rec( cubes, tt, 0, kitty::cube() );

  return esop_t( cubes.begin(), cubes.end() );
}

} // namespace esop

}
