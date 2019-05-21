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

#include <easy/sat/sat_solver.hpp>
#include <easy/utils/dynamic_bitset.hpp>

#include <queue>

namespace easy::sat2
{

class cnf_from_xcnf
{
public:
  explicit cnf_from_xcnf( int& sid, std::vector<std::vector<int>> const& xor_clauses, uint32_t num_vars )
    : _sid( sid )
    , _xor_clauses( xor_clauses )
    , _num_vars( num_vars )
  {
    auto matrix = make_matrix( xor_clauses );
    // simplify_matrix( matrix );

    cnf_from_matrix( _clauses, matrix );
    // cnf_from_xor_clauses( _clauses, xor_clauses );
  }

  std::vector<utils::dynamic_bitset<>> make_matrix( std::vector<std::vector<int>> const& xor_clauses )
  {
    std::vector<utils::dynamic_bitset<>> matrix;
    for ( const auto& cl : xor_clauses )
    {
      utils::dynamic_bitset<> row;
      row.resize( _num_vars );

      auto sum = 0u;
      for ( const auto& l : cl )
      {
        assert( abs( l ) < _num_vars );
        row.set_bit( abs( l ) - 1 );
        sum += l < 0;
      }

      if ( sum % 2 == 1 )
        row.set_bit( row.num_bits() - 1 );

      matrix.emplace_back( row );
    }
    return matrix;
  }

  void simplify_matrix( std::vector<utils::dynamic_bitset<>>& matrix )
  {
    auto const num_rows = matrix.size();

    auto index = 0u;
    for ( auto i = 0u; i < num_rows; ++i )
    {
      /* find next row */
      auto next_row = i;
      if ( !matrix[i][index] )
      {
        for ( auto k = i + 1; k < num_rows; ++k )
        {
          if ( matrix[k][index] )
          {
            ++index;
            continue;
          }
        }

        if ( !matrix[next_row][i] )
        {
          ++index;
          continue;
        }
      }

      /* swap current i and next_row */
      if ( next_row != i )
      {
        // std::cout << "swap " << i << " and " << next_row << std::endl;
        for ( auto j = 0u; j < matrix[i].num_bits(); ++j )
        {
          bool const temp = matrix[i][j];
          if ( matrix[next_row][j] )
            matrix[i].set_bit( j );
          else
            matrix[i].reset_bit( j );

          if ( temp )
            matrix[next_row].set_bit( j );
          else
            matrix[next_row].reset_bit( j );
        }
      }

      /* add current row to all other rows */
      for ( auto k = i + 1; k < num_rows; ++k )
      {
        if ( !matrix[k][index] ) continue;

        // std::cout << "add " << i << " to " << k << std::endl;
        for ( auto j = 0u; j < matrix[k].num_bits(); ++j )
        {
          if ( matrix[k][j] ^ matrix[i][j] )
          {
            matrix[k].set_bit( j );
          }
          else
          {
            matrix[k].reset_bit( j );
          }
        }
      }

      ++index;
    }
  }

  void print_matrix( std::ostream& os, std::vector<utils::dynamic_bitset<>> const& matrix ) const
  {
    for ( const auto& m : matrix )
    {
      for ( auto i = 0; i < m.num_bits(); ++i )
      {
        os << m[i];
      }
      os << std::endl;
    }
  }

  inline void add_xor_clause( std::vector<std::vector<int>>& clauses, const std::vector<int>& xor_clause )
  {
    assert( !xor_clause.empty() && "clause must not be empty" );

    std::queue<int> lits;
    for ( const auto& l : xor_clause )
    {
      lits.push( l );
    }

    while ( lits.size() > 1 )
    {
      auto a = lits.front();
      lits.pop();
      auto b = lits.front();
      lits.pop();

      int c = _sid++;
      clauses.emplace_back( std::vector<int>{ -a, -b, -c } );
      clauses.emplace_back( std::vector<int>{  a,  b, -c } );
      clauses.emplace_back( std::vector<int>{  a, -b,  c } );
      clauses.emplace_back( std::vector<int>{ -a,  b,  c } );

      lits.push( c );
    }

    assert( lits.size() == 1u );
    clauses.emplace_back( std::vector<int>{ lits.front() } );
  }

  void cnf_from_matrix( std::vector<std::vector<int>>& clauses, std::vector<utils::dynamic_bitset<>> const& matrix )
  {
    for ( const auto& row : matrix )
    {
      std::vector<int> clause;
      for ( auto i = 0; i < row.num_bits()-1u; ++i )
      {
        if ( row[i] )
        {
          clause.push_back( i+1 );
        }
      }

      if ( !clause.empty() )
      {
        if ( row[row.num_bits() - 1u] )
        {
          clause[0] *= -1;
        }

        add_xor_clause( _clauses, clause );
      }
    }
  }

  void cnf_from_xor_clauses( std::vector<std::vector<int>>& clauses, std::vector<std::vector<int>> const& xor_clauses )
  {
    for ( const auto& xor_clause : xor_clauses )
    {
      add_xor_clause( _clauses, xor_clause );
    }
  }

  std::vector<std::vector<int>> get() const
  {
    return _clauses;
  }

protected:
  int& _sid;

  std::vector<std::vector<int>> const& _xor_clauses;
  uint32_t _num_vars;

  std::vector<std::vector<int>> _clauses;
}; /* cnf_from_xcnf */

} // namespace easy::sat

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
