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

#include <kitty/cube.hpp>

#include <unordered_set>

namespace easy::esop::detail
{

inline void add_to_cubes( std::unordered_set<kitty::cube, kitty::hash<kitty::cube>>& pkrm, const kitty::cube& c, bool distance_one_merging = true )
{
  /* first check whether cube is already contained; if so, delete it */
  const auto it = pkrm.find( c );
  if ( it != pkrm.end() )
  {
    pkrm.erase( it );
    return;
  }

  /* otherwise, check if there is a distance-1 cube; if so, merge it */
  if ( distance_one_merging )
  {
    for ( auto it = pkrm.begin(); it != pkrm.end(); ++it )
    {
      if ( c.distance( *it ) == 1 )
      {
        auto new_cube = c.merge( *it );
        pkrm.erase( it );
        add_to_cubes( pkrm, new_cube );
        return;
      }
    }
  }

  /* otherwise, just add the cube */
  pkrm.insert( c );
}

inline kitty::cube with_literal( const kitty::cube& c, uint8_t var_index, bool polarity )
{
  auto copy = c;
  copy.add_literal( var_index, polarity );
  return copy;
}

} // namespace easy::esop::detail
