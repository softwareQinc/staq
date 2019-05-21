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

#include <esop/esop.hpp>
#include <kitty/cube.hpp>

/***
 *
 * Combine operations for ESOPs based on
 *
 * Stergios Stergiou, George K. Papakonstantinou: Exact Minimization Of Esop Expressions With Less Than
 * Eight Product Terms. Journal of Circuits, Systems, and Computers 13(1): 1-15, 2004.
 */

namespace easy::esop
{

void simple_combine_inplace( esop_t& expr, uint8_t var_index, uint8_t i )
{
  assert( i >= 0 && i <= 2 );
  if ( i == 2 )
    return;
  for ( auto& c : expr )
  {
    c.add_literal( var_index, i );
  }
}

void simple_combine_inplace( esops_t& esops, uint8_t var_index, uint8_t i )
{
  assert( i >= 0 && i <= 2 );
  if ( i == 2 )
    return;
  for ( auto& e : esops )
  {
    simple_combine_inplace( e, var_index, i );
  }
}

esop_t simple_combine( const esop_t& expr, uint8_t var_index, uint8_t i )
{
  esop_t result = expr;
  simple_combine_inplace( result, var_index, i );
  return result;
}

esops_t simple_combine( const esops_t& esops, uint8_t var_index, uint8_t i )
{
  esops_t result = esops;
  simple_combine_inplace( result, var_index, i );
  return result;
}

esop_t complex_combine( esop_t a, esop_t b, uint8_t var_index, uint8_t i, uint8_t j )
{
  assert( i != j );

  std::vector<kitty::cube> common;
  std::sort( a.begin(), a.end() );
  std::sort( b.begin(), b.end() );
  std::set_intersection( a.begin(), a.end(), b.begin(), b.end(), std::back_inserter( common ) );

  for ( const auto& c : common )
  {
    a.erase( std::remove( a.begin(), a.end(), c ), a.end() );
  }

  for ( const auto& c : common )
  {
    b.erase( std::remove( b.begin(), b.end(), c ), b.end() );
  }

  for ( auto& c : common )
  {
    c.add_literal( var_index, 3 - i - j );
  }

  for ( auto& c : a )
  {
    if ( i != 2 )
    {
      c.add_literal( var_index, i );
    }
    common.push_back( c );
  }

  for ( auto& c : b )
  {
    if ( j != 2 )
    {
      c.add_literal( var_index, j );
    }
    common.push_back( c );
  }

  return common;
}

esops_t complex_combine( const esops_t& as, const esops_t& bs, uint8_t var_index, uint8_t i, uint8_t j )
{
  assert( i >= 0 && i <= 2 );
  assert( j >= 0 && j <= 2 );
  assert( i != j );

  esops_t combinations;
  for ( const auto& a : as )
  {
    for ( const auto& b : bs )
    {
      combinations.push_back( complex_combine( a, b, var_index, i, j ) );
    }
  }

  /* remove duplicates */
  std::sort( combinations.begin(), combinations.end() );
  combinations.erase( std::unique( combinations.begin(), combinations.end() ), combinations.end() );

  return combinations;
}

} // namespace easy::esop

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
