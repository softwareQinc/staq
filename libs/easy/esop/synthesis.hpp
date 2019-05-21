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
#include <easy/esop/exact_synthesis.hpp>
#include <easy/sat/gauss.hpp>
#include <easy/sat/xor_clauses_to_cnf.hpp>
#include <json/json.hpp>

namespace easy::esop
{

namespace detail
{

inline esop_t esop_from_model( const sat::sat_solver::model_t& model, unsigned num_terms, unsigned num_vars )
{
  assert( !( num_terms > 0 ) || ( model.size() != 0 ) );

  esop_t esop;
  for ( auto j = 0u; j < num_terms; ++j )
  {
    kitty::cube c;
    bool cancel_cube = false;
    for ( auto l = 0u; l < num_vars; ++l )
    {
      const auto p_value = model[j * num_vars + l] == l_True;
      const auto q_value = model[num_vars * num_terms + j * num_vars + l] == l_True;

      if ( p_value && q_value )
      {
        cancel_cube = true;
        break;
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
  return esop;
}

} // namespace detail

struct spec
{
  std::string bits;
  std::string care;
}; /* spec */

enum state_t
{
  unknown = 0,
  realizable = 10,
  unrealizable = 20
}; /* state_t */

struct result
{
  result( state_t state = unknown )
      : state( state )
  {
  }

  result( const esop_t& esop )
      : state( realizable ), esop( esop )
  {
  }

  inline operator bool() const { return ( state == realizable ); }

  inline bool is_realizable() const { return ( state == realizable ); }
  inline bool is_unrealizable() const { return ( state == unrealizable ); }
  inline bool is_unknown() const { return ( state == unknown ); }

  state_t state;
  esop_t esop;
}; /* result */

/*! \brief Compute a cover from a truth table.
 *
 * Convert each minterm of the specification into one cube.
 *
 * \param spec Truth-table of a (incompletely-specified) Boolean function
 * \return An ESOP form
 */
inline esop_t esop_cover( const spec& spec )
{
  assert( spec.bits.size() == spec.care.size() );

  const auto size = spec.bits.size();

  esop_t cover;
  for ( auto i = 0u; i < size; ++i )
  {
    if ( spec.care[i] == '1' && spec.bits[i] == '1' )
    {
      cover.emplace_back( kitty::cube( size - 1 - i, size - 1 ) );
    }
  }
  return cover;
}

struct simple_synthesizer_params
{
  /*! A fixed number of product terms (= k) */
  unsigned number_of_terms;
  int conflict_limit = -1;
}; /* simple_synthesizer_params */

/*! \brief Simple ESOP synthesizer
 *
 * Synthesizes a $k$-ESOP given an incompletely-specified Boolean function as specifiction.
 */
class simple_synthesizer
{
public:
  /*! \brief constructor
   *
   * Creates an ESOP synthesizer from a specification.
   *
   * \param spec Truth-table of a(n) (incompletely-specified) Boolean function
   */
  simple_synthesizer( const spec& spec )
      : _spec( spec )
  {
  }

  /*! \brief synthesize
   *
   * SAT-based ESOP synthesis.
   *
   * \params params Parameters
   * \return Synthesis result. If the specification is realizable and
   *         the no resource limit is reached, the result contains an ESOP
   *         that implements the specification.
   */
  result synthesize( const simple_synthesizer_params& params )
  {
    const uint32_t num_vars = log2( _spec.bits.size() );
    assert( _spec.bits.size() == ( 1ull << num_vars ) && "bit-width of bits is not a power of 2" );
    assert( _spec.care.size() == ( 1ull << num_vars ) && "bit-width of care is not a power of 2" );
    assert( num_vars <= 32 && "cube data structure cannot store more than 32 variables" );

    const auto num_terms = params.number_of_terms;
    assert( num_terms >= 1 );

    int sid = 1 + 2 * num_vars * num_terms;

    sat::constraints constraints;
    sat::sat_solver solver;
    if ( params.conflict_limit != -1 )
    {
      solver.set_conflict_limit( params.conflict_limit );
    }

    /* add constraints */
    kitty::cube minterm = kitty::cube::neg_cube( num_vars );

    auto sample_counter = 0u;
    do
    {
      /* skip don't cares */
      if ( ( _spec.bits[minterm._bits] != '0' && _spec.bits[minterm._bits] != '1' ) || _spec.care[minterm._bits] != '1' )
      {
        ++minterm._bits;
        continue;
      }

      std::vector<int> z_vars( num_terms, 0u );
      for ( auto j = 0u; j < num_terms; ++j )
      {
        assert( uint32_t(sid) == 1 + 2 * num_vars * num_terms + sample_counter * num_terms + j );
        z_vars[j] = sid++;
      }

      for ( auto j = 0u; j < num_terms; ++j )
      {
        const int z = z_vars[j];

        // positive
        for ( auto l = 0u; l < num_vars; ++l )
        {
          if ( minterm.get_bit( l ) )
          {
            std::vector<int> clause;
            clause.push_back( -z );                                               // - z_j
            clause.push_back( -( 1 + num_vars * num_terms + num_vars * j + l ) ); // -q_j,l

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

      for ( auto j = 0u; j < num_terms; ++j )
      {
        const int z = z_vars[j];

        // negative
        std::vector<int> clause = {z};
        for ( auto l = 0u; l < num_vars; ++l )
        {
          if ( minterm.get_bit( l ) )
          {
            clause.push_back( 1 + num_vars * num_terms + num_vars * j + l ); // q_j,l
          }
          else
          {
            clause.push_back( 1 + num_vars * j + l ); // p_j,l
          }
        }

        constraints.add_clause( clause );
      }

      constraints.add_xor_clause( z_vars, _spec.bits[minterm._bits] == '1' );

      ++sample_counter;
      ++minterm._bits;
    } while ( minterm._bits < ( 1 << num_vars ) );

    sat::gauss_elimination().apply( constraints );
    sat::xor_clauses_to_cnf( sid ).apply( constraints );

    const auto sat = solver.solve( constraints );
    if ( sat.is_undef() )
    {
      return result();
    }
    else if ( sat.is_unsat() )
    {
      return result( unrealizable );
    }
    else
    {
      assert( sat.is_sat() );
      return result( make_esop( sat.model, num_terms, num_vars ) );
    }
  }

  /*! \brief stats
   *
   * \Return A json log with statistics logged during the synthesis
   */
  nlohmann::json stats() const
  {
    return _stats;
  }

private:
  /*! \brief make_esop
   *
   * Extract the ESOP from a satisfying assignments.
   *
   * \param model Satisfying assignment
   * \param num_terms Number of terms
   * \param num_vars Number of variables
   */
  esop_t make_esop( const sat::sat_solver::model_t& model, unsigned num_terms, unsigned num_vars )
  {
    return detail::esop_from_model( model, num_terms, num_vars );
  }

private:
  const spec _spec;
  nlohmann::json _stats;
}; /* simple_synthesizer */

/*! \brief minimum_synthesizer_params
 *
 * Parameters for the minimum ESOP synthesizer.
 *
 * The parameters begin and next can be customized for upwards or downwards search.
 *
 * Example: (upwards search)
 *   minimum_synthesizer_params params;
 *   params.begin = 0;
 *   params.next = [&]( uint32_t i ){ if ( i >= max_k || sat ) return false; ++i; return true; }
 *
 * Example: (downwards search)
 *   minimum_synthesizer_params params;
 *   params.begin = max_k;
 *   params.next = [&]( uint32_t i ){ if ( i <= 0 || !sat ) return false; --i; return true; }
 *
 */
struct minimum_synthesizer_params
{
  /*! Start value for the search */
  int begin;
  /*! A function that evaluates the current value and satisfiability result, decides whether to
      terminate, and updates the value */
  std::function<bool( uint32_t&, sat::sat_solver::result )> next;
  int conflict_limit = -1;
}; /* minimum_synthesizer_params */

/*! \brief Minimum ESOP synthesizer
 *
 * Similar to simple_synthesizer, but searches for a minimum ESOP in a
 * specified range.
 */
class minimum_synthesizer
{
public:
  /*! \brief constructor
   *
   * Creates an ESOP synthesizer from a specification.
   *
   * \param spec Truth-table of a(n) (incompletely-specified) Boolean function
   */
  minimum_synthesizer( const spec& spec )
      : _spec( spec )
  {
  }

  /*! \brief synthesize
   *
   * SAT-based ESOP synthesis.
   *
   * \params params Parameters
   * \return An ESOP form
   */
  result synthesize( const minimum_synthesizer_params& params )
  {
    const uint32_t num_vars = log2( _spec.bits.size() );
    assert( _spec.bits.size() == ( 1ull << num_vars ) && "bit-width of bits is not a power of 2" );
    assert( _spec.care.size() == ( 1ull << num_vars ) && "bit-width of care is not a power of 2" );
    assert( num_vars <= 32 && "cube data structure cannot store more than 32 variables" );

    esop_t esop;
    sat::sat_solver::result result;
    bool all_unsat = true;

    uint32_t k = params.begin;
    do
    {
      assert( k != 0 && "synthesis of constants not supported" );
      int sid = 1 + 2 * num_vars * k;

      sat::constraints constraints;
      sat::sat_solver solver;

      if ( params.conflict_limit != -1 )
      {
        solver.set_conflict_limit( params.conflict_limit );
      }

      /* add constraints */
      kitty::cube minterm = kitty::cube::neg_cube( num_vars );

      auto sample_counter = 0u;
      do
      {
        /* skip don't cares */
        if ( ( _spec.bits[minterm._bits] != '0' && _spec.bits[minterm._bits] != '1' ) || _spec.care[minterm._bits] != '1' )
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

        constraints.add_xor_clause( z_vars, _spec.bits[minterm._bits] == '1' );

        ++sample_counter;
        ++minterm._bits;
      } while ( minterm._bits < ( 1 << num_vars ) );

      sat::gauss_elimination().apply( constraints );
      sat::xor_clauses_to_cnf( sid ).apply( constraints );
      // sat::cnf_symmetry_breaking( sid ).apply( constraints );

      result = solver.solve( constraints );

      if ( result.is_sat() )
      {
        esop = make_esop( result.model, k, num_vars );
      }

      if ( !result.is_unsat() )
      {
        all_unsat = false;
      }
    } while ( params.next( k, result ) );

    /* no ESOP constructed, either UNSAT or UNREALIZABLE */
    if ( esop.size() == 0u )
    {
      if ( all_unsat )
        return easy::esop::result( unrealizable );
      else
        return easy::esop::result();
    }

    return esop;
  }

  /*! \brief stats
   *
   * \Return A json log with statistics logged during the synthesis
   */
  nlohmann::json stats() const
  {
    return _stats;
  }

private:
  /*! \brief make_esop
   *
   * Extract the ESOP from a satisfying assignments.
   *
   * \param model Satisfying assignment
   * \param num_terms Number of terms
   * \param num_vars Number of variables
   */
  esop_t make_esop( const sat::sat_solver::model_t& model, unsigned num_terms, unsigned num_vars )
  {
    return detail::esop_from_model( model, num_terms, num_vars );
  }

private:
  const spec _spec;
  nlohmann::json _stats;
}; /* minimum_synthesizer */

/*! \brief minimum_synthesizer_params
 *
 * Parameters for the minimum ESOP synthesizer.
 *
 * The parameters begin and next can be customized for upwards or downwards search.
 *
 * Example: (upwards search)
 *   minimum_synthesizer_params params;
 *   params.begin = 0;
 *   params.next = [&]( int i ){ if ( i >= max_k || sat ) return false; ++i; return true; }
 *
 * Example: (downwards search)
 *   minimum_synthesizer_params params;
 *   params.begin = max_k;
 *   params.next = [&]( int i ){ if ( i <= 0 || !sat ) return false; --i; return true; }
 *
 */
struct minimum_all_synthesizer_params
{
  /*! Start value for the search */
  int begin;
  /*! A function that evaluates the current value and satisfiability result, decides whether to
      terminate, and updates the value */
  std::function<bool( uint32_t&, sat::sat_solver::result )> next;
  int conflict_limit = -1;
}; /* minimum_all_synthesizer_params */

/*! \brief Minimum ESOP synthesizer
 *
 * Similar to simple_synthesizer, but searches for a minimum ESOP in a
 * specified range.
 */
class minimum_all_synthesizer
{
public:
  /*! \brief constructor
   *
   * Creates an ESOP synthesizer from a specification.
   *
   * \param spec Truth-table of a(n) (incompletely-specified) Boolean function
   */
  minimum_all_synthesizer( const spec& spec )
      : _spec( spec )
  {
  }

  /*! \brief synthesize
   *
   * SAT-based ESOP synthesis.
   *
   * \params params Parameters
   * \return An ESOP form
   */
  esops_t synthesize( const minimum_all_synthesizer_params& params )
  {
    const uint32_t num_vars = log2( _spec.bits.size() );
    assert( _spec.bits.size() == ( 1ull << num_vars ) && "bit-width of bits is not a power of 2" );
    assert( _spec.care.size() == ( 1ull << num_vars ) && "bit-width of care is not a power of 2" );
    assert( num_vars <= 32 && "cube data structure cannot store more than 32 variables" );

    esop_t esop;
    sat::sat_solver::result result;

    std::unique_ptr<sat::constraints> constraints;
    std::unique_ptr<sat::sat_solver> solver;

    uint32_t k = params.begin;
    do
    {
      int sid = 1 + 2 * num_vars * k;

      constraints.reset( new sat::constraints );
      solver.reset( new sat::sat_solver );

      /* add constraints */
      kitty::cube minterm = kitty::cube::neg_cube( num_vars );

      auto sample_counter = 0u;
      do
      {
        /* skip don't cares */
        if ( ( _spec.bits[minterm._bits] != '0' && _spec.bits[minterm._bits] != '1' ) || _spec.care[minterm._bits] != '1' )
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

              constraints->add_clause( clause );
            }
            else
            {
              std::vector<int> clause;
              clause.push_back( -z );                        // -z_j
              clause.push_back( -( 1 + num_vars * j + l ) ); // -p_j,l

              constraints->add_clause( clause );
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

          constraints->add_clause( clause );
        }

        constraints->add_xor_clause( z_vars, _spec.bits[minterm._bits] == '1' );

        ++sample_counter;
        ++minterm._bits;
      } while ( minterm._bits < ( 1 << num_vars ) );

      sat::gauss_elimination().apply( *constraints );
      sat::xor_clauses_to_cnf( sid ).apply( *constraints );
      // sat::cnf_symmetry_breaking( sid ).apply( *constraints );

      if ( ( result = solver->solve( *constraints ) ) )
      {
        esop = make_esop( result.model, k, num_vars );
      }
    } while ( params.next( k, result ) );

    /* restore the state if the last call has been unsat */
    if ( k < esop.size() )
    {
      k = esop.size();

      int sid = 1 + 2 * num_vars * k;

      constraints.reset( new sat::constraints );
      solver.reset( new sat::sat_solver );

      /* add constraints */
      kitty::cube minterm = kitty::cube::neg_cube( num_vars );

      auto sample_counter = 0u;
      do
      {
        /* skip don't cares */
        if ( ( _spec.bits[minterm._bits] != '0' && _spec.bits[minterm._bits] != '1' ) || _spec.care[minterm._bits] != '1' )
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

              constraints->add_clause( clause );
            }
            else
            {
              std::vector<int> clause;
              clause.push_back( -z );                        // -z_j
              clause.push_back( -( 1 + num_vars * j + l ) ); // -p_j,l

              constraints->add_clause( clause );
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

          constraints->add_clause( clause );
        }

        constraints->add_xor_clause( z_vars, _spec.bits[minterm._bits] == '1' );

        ++sample_counter;
        ++minterm._bits;
      } while ( minterm._bits < ( 1 << num_vars ) );

      sat::gauss_elimination().apply( *constraints );
      sat::xor_clauses_to_cnf( sid ).apply( *constraints );
      // sat::cnf_symmetry_breaking( sid ).apply( *constraints );
    }

    /* enumerate solutions */
    esops_t esops;
    while ( auto result = solver->solve( *constraints ) )
    {
      std::vector<int> blocking_clause;
      std::vector<unsigned> vs;
      vs.resize( k );
      for ( auto i = 0u; i < k; ++i )
      {
        vs[i] = i;
      }

      esop = make_esop( result.model, k, num_vars );

      std::sort( esop.begin(), esop.end(), cube_weight_compare( num_vars ) );

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

            /* do not consider all possibilities for canceled cubes */
            if ( p_value && q_value )
            {
              constraints->add_clause( {int( -( 1 + vs[j] * num_vars + l ) ), int( -( 1 + num_vars * k + vs[j] * num_vars + l ) )} );
              continue;
            }

            blocking_clause.push_back( p_value ? -( 1 + vs[j] * num_vars + l ) : ( 1 + vs[j] * num_vars + l ) );
            blocking_clause.push_back( q_value ? -( 1 + num_vars * k + vs[j] * num_vars + l ) : ( 1 + num_vars * k + vs[j] * num_vars + l ) );
          }
        }
        constraints->add_clause( blocking_clause );
      } while ( std::next_permutation( vs.begin(), vs.end() ) );

      if ( esop.size() < k )
        continue;
      esops.push_back( esop );
    }

    return esops;
  }

  /*! \brief stats
   *
   * \Return A json log with statistics logged during the synthesis
   */
  nlohmann::json stats() const
  {
    return _stats;
  }

private:
  /*! \brief make_esop
   *
   * Extract the ESOP from a satisfying assignments.
   *
   * \param model Satisfying assignment
   * \param num_terms Number of terms
   * \param num_vars Number of variables
   */
  esop_t make_esop( const sat::sat_solver::model_t& model, unsigned num_terms, unsigned num_vars )
  {
    return detail::esop_from_model( model, num_terms, num_vars );
  }

private:
  const spec _spec;
  nlohmann::json _stats;
}; /* minimum_all_synthesizer */

} // namespace easy::esop

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
