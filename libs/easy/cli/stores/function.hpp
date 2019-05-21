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

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>

namespace alice
{

struct function_storee
{
  kitty::dynamic_truth_table bits;
  kitty::dynamic_truth_table care;
  uint32_t number_of_variables;

  std::string as_string() const
  {
    const auto len = 1ull << number_of_variables;
    std::string s( len, '0' );
    for ( auto i = 0u; i < len; ++i )
    {
      if ( !get_bit( care, i ) )
      {
        s[i] = '-';
      }
      else
      {
        s[i] = get_bit( bits, i ) ? '1' : '0';
      }
    }
    return s;
  }

  inline static function_storee from_hex_string( std::string s, uint32_t num_vars )
  {
    /* remove prefix '0x' if present */
    if ( s.size() >= 2 && s[0] == '0' && s[1] == 'x' )
    {
      s = s.substr( 2, s.size()-3 );
    }

    function_storee fn;
    fn.bits = kitty::dynamic_truth_table( num_vars );
    kitty::create_from_hex_string( fn.bits, s );
    fn.care = ~kitty::dynamic_truth_table( num_vars );
    fn.number_of_variables = num_vars;

    return fn;
  }
}; // function_storee

ALICE_ADD_STORE( function_storee, "function", "f", "Function", "Functions" )

ALICE_PRINT_STORE( function_storee, os, element )
{
  kitty::print_hex( element.bits, os );
  os << ' ';
  kitty::print_hex( element.care, os );
  os << '\n';
}

ALICE_DESCRIBE_STORE( function_storee, element )
{
  return fmt::format( "{}", element.as_string() );
}

ALICE_PRINT_STORE_STATISTICS( function_storee, os, element )
{
  os << fmt::format( "{}", element.as_string() ) << '\n';
}

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
