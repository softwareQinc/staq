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

#ifdef __clang__ // CLANG compiler
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wsign-compare"
#include <glucose/glucose.hpp>
#pragma clang diagnostic pop
#elif __GNUC__ // GCC compiler
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <glucose/glucose.hpp>
#pragma GCC diagnostic pop
#else // other compilers
#include <glucose/glucose.hpp>
#endif

#include <cassert>
#include <memory>
#include <vector>
#include <iostream>

namespace easy::sat
{

struct xor_clause_t
{
  std::vector<int> clause;
  bool value;
};

struct wclause_t
{
  std::vector<int> clause;
  uint32_t weight;
};

struct wxor_clause_t
{
  xor_clause_t xor_clause;
  uint32_t weight;
};

class constraints
{
public:
  using weight_t = uint32_t;
  using clause_t = std::vector<int>;

public:
  constraints( weight_t top_weight = 0 )
    : _top_weight( top_weight )
  {
  }

  weight_t top_weight() const
  {
    return _top_weight;
  }

  void set_num_variables( uint32_t nv )
  {
    _num_variables = nv;
  }
  
  uint32_t num_variables() const
  {
    return _num_variables;
  }

  uint32_t num_clauses() const
  {
    return _clauses.size();
  }

  uint32_t num_xor_clauses() const
  {
    return _xor_clauses.size();
  }
  
  void add_clause( const clause_t& clause )
  {
    add_weighted_clause( wclause_t{clause, _top_weight} );
  }

  void add_xor_clause( const clause_t& clause, bool value = true )
  {
    add_weighted_xor_clause( wclause_t{clause, _top_weight}, value );
  }

  void clear_clauses()
  {
    _clauses.clear();
  }

  void clear_xor_clauses()
  {
    _xor_clauses.clear();
  }

  void add_weighted_clause( const clause_t& cl, uint32_t weight )
  {
    add_weighted_clause( {cl, weight} );
  }
  
  void add_weighted_clause( const wclause_t& cl )
  {
    for ( const auto& c : cl.clause )
    {
      uint32_t v = abs( c );
      if ( v > _num_variables )
      {
        _num_variables = v;
      }
    }
    _clauses.emplace_back( cl );
  }

  void add_weighted_xor_clause( const clause_t& cl, bool value, uint32_t weight )
  {
    add_weighted_xor_clause( {cl, weight}, value );
  }
  
  void add_weighted_xor_clause( const wclause_t& cl, bool value = true )
  {
    for ( const auto& c : cl.clause )
    {
      uint32_t v = abs( c );
      if ( v > _num_variables )
      {
        _num_variables = v;
      }
    }
    xor_clause_t xor_clause{cl.clause, value};
    _xor_clauses.emplace_back( wxor_clause_t{xor_clause, cl.weight} );
  }

  template<typename Fn>
  void foreach_clause( Fn const& fn )
  {
    for ( const auto& c : _clauses )
    {
      fn( c.clause );
    }
  }

  template<typename Fn>
  void foreach_clause( Fn const& fn ) const
  {
    for ( const auto& c : _clauses )
    {
      fn( c.clause );
    }
  }

  template<typename Fn>
  void foreach_xor_clause( Fn const& fn )
  {
    for ( const auto& c : _xor_clauses )
    {
      fn( c.xor_clause );
    }
  }

  template<typename Fn>
  void foreach_xor_clause( Fn const& fn ) const
  {
    for ( const auto& c : _xor_clauses )
    {
      fn( c.xor_clause );
    }
  }

  template<typename Fn>
  void foreach_weighted_clause( Fn const& fn )
  {
    for ( const auto& c : _clauses )
    {
      fn( c.clause, c.weight );
    }
  }

  template<typename Fn>
  void foreach_weighted_clause( Fn const& fn ) const
  {
    for ( const auto& c : _clauses )
    {
      fn( c.clause, c.weight );
    }
  }

  template<typename Fn>
  void foreach_weighted_xor_clause( Fn const& fn )
  {
    for ( const auto& c : _xor_clauses )
    {
      fn( c.xor_clause, c.weight );
    }
  }

  template<typename Fn>
  void foreach_weighted_xor_clause( Fn const& fn ) const
  {
    for ( const auto& c : _xor_clauses )
    {
      fn( c.xor_clause, c.weight );
    }
  }

protected:
  weight_t _top_weight;
  std::vector<wclause_t> _clauses;
  std::vector<wxor_clause_t> _xor_clauses;
  uint32_t _num_variables = 0;
}; /* constraints */

} // namespace easy::sat

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
