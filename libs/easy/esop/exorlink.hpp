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
#include <cassert>

namespace easy::esop
{

static unsigned cube_groups2[8] = {
    /* 0 */ 2, 0, 1, 2,
    /* 4 */ 0, 2, 2, 1};

static unsigned cube_groups3[54] = {
    /*  0 */ 2, 0, 0, 1, 2, 0, 1, 1, 2,
    /*  9 */ 2, 0, 0, 1, 0, 2, 1, 2, 1,
    /* 18 */ 0, 2, 0, 2, 1, 0, 1, 1, 2,
    /* 27 */ 0, 2, 0, 0, 1, 2, 2, 1, 1,
    /* 36 */ 0, 0, 2, 2, 0, 1, 1, 2, 1,
    /* 45 */ 0, 0, 2, 0, 2, 1, 2, 1, 1};

#include "exorlink4.def"
#include "exorlink5.def"
#include "exorlink6.def"

/*! \brief EXORLINK cube transformation
 *
 * Transform two cubes with distance into a functionally equivalent set of cubes.
 *
 * \param c0 First cube
 * \param c1 Second cube
 * \param distance Distance of ``c0`` and ``c1``. Must be less than 4.
 * \param group A group of cube transformations
 * \return An array of up to 5 new cubes which are functionally equivalent to ``c0`` and ``c1``.
 */
std::vector<kitty::cube> exorlink( kitty::cube c0, kitty::cube c1, std::uint32_t distance, std::uint32_t* group )
{
  const auto diff = c0.difference( c1 );

  std::vector<kitty::cube> result( distance );
  if ( c1 < c0 )
    std::swap( c0, c1 );

  const auto bits = ~( c0._bits ) & ~( c1._bits );
  const auto mask = c0._mask ^ c1._mask;

  for ( auto i = 0u; i < distance; ++i )
  {
    auto tmp_bits = c0._bits;
    auto tmp_mask = c0._mask;
    auto tmp_pos = diff;

    for ( auto j = 0u; j < distance; ++j )
    {
      /* compute next position */
      std::uint64_t p = tmp_pos & -tmp_pos;
      tmp_pos &= tmp_pos - 1;
      switch ( *group++ )
      {
      case 0:
        /* take from c0 */
        break;
      case 1:
        /* take from c1 */
        tmp_bits ^= ( ( c1._bits & p ) ^ tmp_bits ) & p;
        tmp_mask ^= ( ( c1._mask & p ) ^ tmp_mask ) & p;
        break;
      case 2:
        /* take other */
        tmp_bits ^= ( ( bits & p ) ^ tmp_bits ) & p;
        tmp_mask ^= ( ( mask & p ) ^ tmp_mask ) & p;
        break;
      }
    }
    result[i]._bits = tmp_bits;
    result[i]._mask = tmp_mask;
  }

  return result;
}

/*! \brief EXORLINK4 cube transformation
 *
 * Transform two cubes with distance 4 into a functionally equivalent set of 4 other cubes.
 *
 * \param c0 First cube
 * \param c1 Second cube
 * \param offset An offset that determines the transformation (must be a value in the series 0, 16, 32, ..., 368)
 * \return An array of 4 new cubes which are functionally equivalent to ``c0`` and ``c1``.
 */
std::array<kitty::cube, 4> exorlink4( const kitty::cube& c0, const kitty::cube& c1, uint32_t offset )
{
  std::uint32_t* group = &cube_groups4[offset];
  const auto diff = c0.difference( c1 );

  std::array<kitty::cube, 4> result;
  const auto bits = ~( c0._bits ) & ~( c1._bits );
  const auto mask = c0._mask ^ c1._mask;

  if ( c0 < c1 )
  {
    for ( auto i = 0; i < 4; ++i )
    {
      auto tmp_bits = c0._bits;
      auto tmp_mask = c0._mask;
      auto tmp_pos = diff;

      for ( auto j = 0; j < 4; ++j )
      {
        /* compute next position */
        std::uint64_t p = tmp_pos & -tmp_pos;
        tmp_pos &= tmp_pos - 1;
        switch ( *group++ )
        {
        case 0:
          /* take from c0 */
          break;
        case 1:
        {
          /* take from c1 */
          tmp_bits ^= ( ( c1._bits & p ) ^ tmp_bits ) & p;
          tmp_mask ^= ( ( c1._mask & p ) ^ tmp_mask ) & p;
        }
        break;
        case 2:
          /* take other */
          tmp_bits ^= ( ( bits & p ) ^ tmp_bits ) & p;
          tmp_mask ^= ( ( mask & p ) ^ tmp_mask ) & p;
          break;
        }
      }
      result[i]._bits = tmp_bits;
      result[i]._mask = tmp_mask;
    }
  }
  else
  {
    for ( auto i = 0; i < 4; ++i )
    {
      auto tmp_bits = c1._bits;
      auto tmp_mask = c1._mask;
      auto tmp_pos = diff;

      for ( auto j = 0; j < 4; ++j )
      {
        /* compute next position */
        std::uint64_t p = tmp_pos & -tmp_pos;
        tmp_pos &= tmp_pos - 1;
        switch ( *group++ )
        {
        case 0:
          /* take from c0 */
          break;
        case 1:
          /* take from c1 */
          tmp_bits ^= ( ( c0._bits & p ) ^ tmp_bits ) & p;
          tmp_mask ^= ( ( c0._mask & p ) ^ tmp_mask ) & p;
          break;
        case 2:
          /* take other */
          tmp_bits ^= ( ( bits & p ) ^ tmp_bits ) & p;
          tmp_mask ^= ( ( mask & p ) ^ tmp_mask ) & p;
          break;
        }
      }
      result[i]._bits = tmp_bits;
      result[i]._mask = tmp_mask;
    }
  }

  return result;
}

} // namespace easy::esop

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
