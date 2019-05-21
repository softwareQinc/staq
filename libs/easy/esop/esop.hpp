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

#include <kitty/constructors.hpp>
#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>
#include <cassert>
#include <vector>

namespace easy::esop
{

namespace detail
{
static constexpr auto XOR_SYMBOL = "\u2295";
}

using esop_t = std::vector<kitty::cube>;
using esops_t = std::vector<esop_t>;

/*! \brief Minimum pairwise distance
 *
 * Compute the minimum pairwise distance between ESOP terms.
 *
 * \param esop ESOP
 * \return Minimum pairwise distance
 */
inline unsigned min_pairwise_distance( const esop_t& esop )
{
  auto min = std::numeric_limits<unsigned>::max();
  for ( auto i = 0u; i < esop.size(); ++i )
  {
    for ( auto j = i + 1; j < esop.size(); ++j )
    {
      const auto d = esop[i].distance( esop[j] );
      if ( unsigned(d) < min )
      {
        min = d;
      }
    }
  }
  return min;
}

/*! \brief Maximum pairwise distance
 *
 * Compute the maximum pairwise distance between ESOP terms.
 *
 * \param esop ESOP
 * \return maximum pairwise distance
 */
inline unsigned max_pairwise_distance( const esop_t& esop )
{
  unsigned max = 0;
  for ( auto i = 0u; i < esop.size(); ++i )
  {
    for ( auto j = i + 1; j < esop.size(); ++j )
    {
      const auto d = esop[i].distance( esop[j] );
      if ( unsigned(d) > max )
      {
        max = d;
      }
    }
  }
  return max;
}

/*! \brief Average pairwise distance
 *
 * Compute the average pairwise distance between ESOP terms.
 *
 * \param esop ESOP
 * \return average pairwise distance
 */
inline double avg_pairwise_distance( const esop_t& esop )
{
  double dist = 0;
  auto counter = 0;
  for ( auto i = 0u; i < esop.size(); ++i )
  {
    for ( auto j = i + 1; j < esop.size(); ++j )
    {
      const auto d = esop[i].distance( esop[j] );
      dist += d;
      ++counter;
    }
  }
  return dist / counter;
}

/*! \brief Verify ESOP.
 *
 * Verify ESOP given an incompletely-specified Boolean function as specification.
 *
 * \param esop ESOP
 * \param bits output as Boolean function
 * \param care care-set as Boolean function
 * \return true if ESOP and bits are equal within the care set, or false otherwise
 */
inline bool verify_esop( const esop_t& esop, const std::string& bits, const std::string& care )
{
  assert( bits.size() == care.size() );
  const auto number_of_variables = unsigned( log2( bits.size() ) );

  kitty::dynamic_truth_table tt( number_of_variables );
  kitty::create_from_cubes( tt, esop, true );

  for ( auto i = 0u; i < bits.size(); ++i )
  {
    if ( care[i] && bits[i] != '0' + char(get_bit( tt, i )) )
    {
      return false;
    }
  }
  return true;
}

/*! \brief Check ESOP equivalence of two ESOP forms.
 *
 * Check if two given ESOP forms are functionally equivalent.
 *
 * \param esop1 First ESOP
 * \param esop2 Second ESOP
 * \param num_vars Number of Boolean variables
 * \return true if esop1 == esop2 and false otherwise
 */
inline bool equivalent_esops( const esop_t& esop1, const esop_t& esop2, unsigned num_vars )
{
  assert( num_vars <= 20 && "20 and more variables cannot be handled using explicit truth table manipulation" );

  kitty::dynamic_truth_table tt1( num_vars );
  kitty::create_from_cubes( tt1, esop1, true );

  kitty::dynamic_truth_table tt2( num_vars );
  kitty::create_from_cubes( tt2, esop2, true );

  return tt1 == tt2;
}

/*! \brief Check if ESOP form implements an incompletely-specified Boolean function
 *
 * Check if an ESOP form implements an incompletely-specified Boolean function.
 *
 * \param esop ESOP form
 * \param bits Boolean function
 * \param care Care function
 * \param num_vars Number of Boolean variables
 * \return true if esop element Impl(bits,care) and false otherwise
 */
inline bool implements_function( const esop_t& esop, const kitty::dynamic_truth_table& bits, const kitty::dynamic_truth_table& care, unsigned num_vars )
{
  assert( num_vars <= 20 && "20 and more variables cannot be handled using explicit truth table manipulation" );

  kitty::dynamic_truth_table tt( num_vars );
  kitty::create_from_cubes( tt, esop, true );

  return ( tt & care ) == ( bits & care );
}

/*! \brief Printer function for ESOP
 *
 * Print ESOP as an expression.
 *
 * \param esop ESOP
 * \param num_vars Number of variables
 * \param os Output stream
 */
inline void print_esop_as_exprs( const esop_t& esop, unsigned num_vars, std::ostream& os = std::cout )
{
  assert( num_vars <= 32 );
  os << esop.size() << ' ';
  for ( auto i = 0u; i < esop.size(); ++i )
  {
    const auto& c = esop[i];
    auto lit_count = c.num_literals();
    if ( lit_count == 0 )
    {
      os << "(1)";
    }
    else
    {
      os << "(";
      for ( auto j = 0u; j < num_vars; ++j )
      {
        if ( ( c._mask >> j ) & 1 )
        {
          os << ( ( ( c._bits >> j ) & 1 ) ? "x" : "~x" ) << j;
          --lit_count;
          if ( lit_count != 0 )
          {
            os << "*";
          }
        }
      }
      os << ")";
    }
    if ( i + 1 < esop.size() )
    {
      os << detail::XOR_SYMBOL;
    }
  }
  os << '\n';
}

/*! \brief Printer function for ESOP
 *
 * Print ESOP as a list of cubes.
 *
 * \param esop ESOP
 * \param num_vars Number of variables
 * \param os Output stream
 */
inline void print_esop_as_cubes( const esop_t& esop, unsigned num_vars, std::ostream& os = std::cout )
{
  assert( num_vars <= 32 );
  for ( const auto& c : esop )
  {
    c.print( num_vars, os );
    os << ' ';
  }
  os << '\n';
}

} /* namespace easy::esop */

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
