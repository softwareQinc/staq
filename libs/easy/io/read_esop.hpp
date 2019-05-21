/* easy: C++ ESOP library
 * Copyright (C) 2017-2018  EPFL
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
#include <lorina/pla.hpp>

namespace easy
{

/*! \brief lorina reader callback for PLA files
 *
 * Reads a PLA file and stores the terms as esop_t.
 *
 */
class esop_storage_reader : public lorina::pla_reader
{
public:
  esop_storage_reader( esop::esop_t& esop, unsigned& num_vars )
      : _esop( esop ), _num_vars( num_vars )
  {
  }

  void on_number_of_inputs( std::size_t i ) const override
  {
    _num_vars = i;
  }

  void on_term( const std::string& term, const std::string& out ) const override
  {
    assert( out == "1" );
    _esop.emplace_back( term );
  }

  bool on_keyword( const std::string& keyword, const std::string& value ) const override
  {
    if ( keyword == "type" && value == "esop" )
    {
      return true;
    }
    return false;
  }

  esop::esop_t& _esop;
  unsigned& _num_vars;
}; /* esop_storage_reader */

} /* namespace easy */

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
