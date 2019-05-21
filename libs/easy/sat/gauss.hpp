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

namespace easy::sat
{

class gauss_elimination
{
public:
  gauss_elimination() = default;

  bool apply( constraints& constraints )
  {
    auto A = make_matrix( constraints );
    matrix_make_upper_triangular_binary( A );

    /* reconstruct xor_clauses */
    constraints.clear_xor_clauses();

    for ( const auto& row : A )
    {
      std::vector<int> clause;
      for ( auto i = 0; i < int( row.size() - 1u ); ++i )
      {
        if ( row[i] )
        {
          clause.push_back( i + 1 );
        }
      }

      if ( clause.empty() && row[ row.size() - 1u ] )
      {
        constraints.clear_clauses();
        constraints.add_clause( {  1 } );
        constraints.add_clause( { -1 } );
        return true;
      }
      else if ( !clause.empty() )
      {
        constraints.add_weighted_xor_clause( sat::wclause_t{clause, 0u}, row[row.size() - 1u] );
      }
    }

    return false;
  }

protected:
  inline std::vector<std::vector<bool>> make_matrix( const constraints& constraints )
  {
    auto num_variables = constraints.num_variables();
    std::vector<std::vector<bool>> matrix;
    constraints.foreach_xor_clause( [&]( xor_clause_t const& cl ){
        std::vector<bool> row( num_variables + 1 );
        auto sum = 0u;
        for ( const auto& l : cl.clause )
        {
          row[abs( l ) - 1] = 1;
          sum += l < 0;
        }
        sum += cl.value;
        row[row.size() - 1] = sum % 2;
        matrix.emplace_back( row );
      });
    return matrix;
  }

  inline void matrix_make_upper_triangular_binary( std::vector<std::vector<bool>>& A )
  {
    const auto num_rows = A.size();
    auto index = 0u;
    for ( auto i = 0u; i < num_rows; ++i )
    {
      /* find next row */
      auto next_row = i;
      if ( !A[i][index] )
      {
        for ( auto k = i + 1; k < num_rows; ++k )
        {
          if ( A[k][index] )
          {
            next_row = k;
          }
        }

        if ( !A[next_row][i] )
        {
          ++index;
          continue;
        }
      }

      /* swap current i and next_row */
      if ( next_row != i )
      {
        // std::cout << "swap " << i << " and " << next_row << std::endl;
        for ( auto j = 0u; j < A[i].size(); ++j )
        {
          bool temp = A[i][j];
          A[i][j] = A[next_row][j];
          A[next_row][j] = temp;
        }
      }

      // add current row to all other rows
      for ( auto k = i + 1; k < num_rows; ++k )
      {
        if ( !A[k][index] )
          continue;
        // std::cout << "add " << i << " to " << k << std::endl;
        for ( auto j = 0u; j < A[k].size(); ++j )
        {
          A[k][j] = A[k][j] ^ ( A[i][j] );
        }
      }

      ++index;
    }
  }
}; /* gauss_elimination */

} // namespace easy::sat

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
