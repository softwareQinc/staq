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
#include <easy/esop/cube_utils.hpp>
#include <easy/sat/sat_solver.hpp>
#include <easy/sat/gauss.hpp>
#include <easy/sat/xor_clauses_to_cnf.hpp>
#include <easy/sat/cnf_writer.hpp>
#include <easy/utils/string_utils.hpp>
#include <json/json.hpp>
#include <fmt/format.h>
#include <fstream>

namespace easy::esop
{

namespace detail
{

inline esops_t exact_synthesis_from_binary_string( const std::string& bits, const std::string& care, const nlohmann::json& config )
{
  const auto max_number_of_cubes = ( config.count( "maximum_cubes" ) > 0u ? unsigned( config["maximum_cubes"] ) : 10 );
  const auto dump = ( config.count( "dump_cnf" ) > 0u ? bool( config["dump_cnf"] ) : false );
  const auto one_esop = ( config.count( "one_esop" ) > 0u ? bool( config["one_esop"] ) : true );

  const uint32_t num_vars = log2( bits.size() );
  assert( bits.size() == ( 1ull << num_vars ) && "bit-width of bits is not a power of 2" );
  assert( care.size() == ( 1ull << num_vars ) && "bit-width of care is not a power of 2" );
  assert( num_vars <= 32 && "cube data structure cannot store more than 32 variables" );

  esop::esops_t esops;
  for ( auto k = 1u; k <= max_number_of_cubes; ++k )
  {
    // std::cout << "[i] bounded synthesis for k = " << k << std::endl;
    int sid = 1 + 2 * num_vars * k;

    sat::constraints constraints;
    sat::sat_solver solver;

    /* add constraints */
    kitty::cube minterm = kitty::cube::neg_cube( num_vars );

    auto sample_counter = 0u;
    do
    {
      /* skip don't cares */
      if ( ( bits[minterm._bits] != '0' && bits[minterm._bits] != '1' ) || care[minterm._bits] != '1' )
      {
        ++minterm._bits;
        continue;
      }

      std::vector<int> z_vars( k, 0u );
      for ( auto j = 0u; j < k; ++j )
      {
        assert( uint32_t(sid) == 1 + 2 * num_vars * k + sample_counter * k + j );
        z_vars[j] = sid++;
      }

      for ( auto j = 0u; j < k; ++j )
      {
        const int z = z_vars[j];

        // positive
        for ( auto l = 0u; l < num_vars; ++l )
        {
          if ( minterm.get_bit( l ) )
          {
            std::vector<int> clause;
            clause.push_back( -z );                                       // - z_j
            clause.push_back( -( 1 + num_vars * k + num_vars * j + l ) ); // -q_j,l

            constraints.add_clause( clause );
          }
          else
          {
            std::vector<int> clause;
            clause.push_back( -z );                        // -z_j
            clause.push_back( -( 1 + num_vars * j + l ) ); // -p_j,l

            constraints.add_clause( clause );
          }
        }
      }

      for ( auto j = 0u; j < k; ++j )
      {
        const int z = z_vars[j];

        // negative
        std::vector<int> clause = {z};
        for ( auto l = 0u; l < num_vars; ++l )
        {
          if ( minterm.get_bit( l ) )
          {
            clause.push_back( 1 + num_vars * k + num_vars * j + l ); // q_j,l
          }
          else
          {
            clause.push_back( 1 + num_vars * j + l ); // p_j,l
          }
        }

        constraints.add_clause( clause );
      }

      constraints.add_xor_clause( z_vars, bits[minterm._bits] == '1' );

      ++sample_counter;
      ++minterm._bits;
    } while ( minterm._bits < ( 1 << num_vars ) );

    assert( sample_counter > 0 );

    sat::gauss_elimination().apply( constraints );
    sat::xor_clauses_to_cnf( sid ).apply( constraints );

    if ( dump )
    {
      const std::string filename = fmt::format( "0x%s-%d.cnf", utils::hex_string_from_binary_string( bits ), k );
      std::cout << "[i] write CNF file " << filename << std::endl;
      std::ofstream os( filename );
      {
        sat::cnf_writer writer( os );
        writer.apply( constraints );
        os.close();
      }
    }

    while ( auto result = solver.solve( constraints ) )
    {
      esop::esop_t esop;

      std::vector<int> blocking_clause;

      std::vector<unsigned> vs;
      vs.resize( k );
      for ( auto i = 0u; i < k; ++i )
      {
        vs[i] = i;
      }

      /* extract the esop from the satisfying assignments */
      for ( auto j = 0u; j < k; ++j )
      {
        kitty::cube c;
        bool cancel_cube = false;
        for ( auto l = 0u; l < num_vars; ++l )
        {
          const auto p_value = result.model[j * num_vars + l] == l_True;
          const auto q_value = result.model[num_vars * k + j * num_vars + l] == l_True;

          if ( p_value && q_value )
          {
            cancel_cube = true;
          }
          else if ( p_value )
          {
            c.add_literal( l, true );
          }
          else if ( q_value )
          {
            c.add_literal( l, false );
          }
        }

        if ( cancel_cube )
        {
          continue;
        }

        esop.push_back( c );
      }

      /* special case: if the empty ESOP, i.e., false, is a possible solution, the immediately return */
      if ( esop.empty() )
      {
        return {esop};
      }

      /* if only one ESOP should be computed then terminate */
      if ( one_esop )
      {
        return {esop};
      }

      std::sort( esop.begin(), esop.end(), cube_weight_compare( num_vars ) );
      esops.push_back( esop );

      /* add one blocking clause for each possible permutation of the cubes */
      do
      {
        std::vector<int> blocking_clause;
        for ( auto j = 0u; j < vs.size(); ++j )
        {
          for ( auto l = 0u; l < num_vars; ++l )
          {
            const auto p_value = result.model[j * num_vars + l] == l_True;
            const auto q_value = result.model[num_vars * k + j * num_vars + l] == l_True;

            blocking_clause.push_back( p_value ? -( 1 + vs[j] * num_vars + l ) : ( 1 + vs[j] * num_vars + l ) );
            blocking_clause.push_back( q_value ? -( 1 + num_vars * k + vs[j] * num_vars + l ) : ( 1 + num_vars * k + vs[j] * num_vars + l ) );
          }
        }

        constraints.add_clause( blocking_clause );
      } while ( std::next_permutation( vs.begin(), vs.end() ) );
    }

    if ( esops.size() > 0 )
    {
      break;
    }
  }

  return esops;
}

} /* detail */

template<typename TT>
esops_t exact_esop( TT const& bits )
{
  auto const size = 1u << bits.num_vars();
  std::string bs( size, '-' );
  std::string cs( size, '-' );
  for ( auto i = 0; i < size; ++i )
  {
    bs[i] = kitty::get_bit( bits, i ) ? '1' : '0';
    cs[i] = '1';
  }

  nlohmann::json config;
  config["one_esop"] = false;
  return detail::exact_synthesis_from_binary_string( bs, cs, config );
}

} /* namespace easy::esop */

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
