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
#include <ostream>
#include <sstream>

namespace easy
{

/*! \brief Writes ESOP form in PLA format to output stream
 *
 * \param os Output stream
 * \param esop ESOP form
 * \param num_vars Number of variables
 */
inline void write_esop( std::ostream& os, esop::esop_t const& esop, unsigned num_vars )
{
  lorina::pla_writer writer( os );
  writer.on_number_of_inputs( num_vars );
  writer.on_number_of_outputs( 1 );
  writer.on_number_of_terms( esop.size() );
  writer.on_keyword( "type", "esop" );
  for ( const auto& e : esop )
  {
    std::stringstream ss;
    e.print( num_vars, ss );
    writer.on_term( ss.str(), "1" );
  }
  writer.on_end();
}

/*! \brief Writes ESOP form in PLA format to a file
 *
 * \param filename Name of the file to be written
 * \param esop ESOP form
 * \param num_vars Number of variables
 */
inline void write_esop( std::string const& filename, esop::esop_t const& esop, unsigned num_vars )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_esop( os, esop, num_vars );
  os.close();
}

} /* namespace easy */

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
