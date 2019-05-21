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

#include <easy/esop/esop.hpp>

namespace alice
{

struct esop_storee
{
  std::string model_name;
  easy::esop::esop_t esop;
  std::size_t number_of_inputs;
  std::size_t number_of_outputs;
}; /* esop_storee */

ALICE_ADD_STORE( esop_storee, "esop", "e", "ESOP", "ESOPs" )

ALICE_PRINT_STORE( esop_storee, os, element )
{
  auto i = 0;
  for ( const auto& c : element.esop )
  {
    os << ( i++ ) << ". ";
    c.print( element.number_of_inputs );
    os << '\n';
  }
}

ALICE_DESCRIBE_STORE( esop_storee, element )
{
  return fmt::format( "[i] esop<{}>: vars={} cubes={}\n", element.model_name, element.number_of_inputs, element.esop.size() );
}

ALICE_PRINT_STORE_STATISTICS( esop_storee, os, element )
{
  os << fmt::format( "[i] esop<{}>: vars={} cubes={}\n", element.model_name, element.number_of_inputs, element.esop.size() );
}

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
