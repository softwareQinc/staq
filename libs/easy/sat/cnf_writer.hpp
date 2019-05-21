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
#include <iostream>

namespace easy::sat
{

class cnf_writer
{
public:
  cnf_writer( std::ostream& os = std::cout )
      : _os( os )
  {
  }

  inline void apply( constraints& constraints )
  {
    _os << "p cnf " << constraints.num_variables() << " " << ( constraints.num_clauses() + constraints.num_xor_clauses() ) << std::endl;
    constraints.foreach_clause( [&]( constraints::clause_t const& cl ){
        for ( const auto& l : cl )
        {
          _os << l << ' ';
        }
        _os << '0' << std::endl;
      });

    constraints.foreach_xor_clause( [&]( xor_clause_t const& cl ){
        if ( cl.clause.size() > 1 )
        {
          _os << "x";
        }
        for ( auto i = 0u; i < cl.clause.size(); ++i )
        {
          if ( !cl.value && i == cl.clause.size() - 1 )
          {
            _os << "-";
          }
          _os << cl.clause[i] << ' ';
        }
        _os << "0" << std::endl;
      });
  }

protected:
  std::ostream& _os;
};

} // namespace easy::sat

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
