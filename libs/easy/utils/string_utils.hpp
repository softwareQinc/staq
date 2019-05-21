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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <locale>
#include <string>

/*
 * This code is based on
 * https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
 */
namespace easy::utils
{

inline void ltrim( std::string& s )
{
  s.erase( s.begin(), std::find_if( s.begin(), s.end(), []( int ch ) {
             return !std::isspace( ch );
           } ) );
}

inline void rtrim( std::string& s )
{
  s.erase( std::find_if( s.rbegin(), s.rend(), []( int ch ) {
             return !std::isspace( ch );
           } )
               .base(),
           s.end() );
}

inline void trim( std::string& s )
{
  ltrim( s );
  rtrim( s );
}

inline std::string ltrim_copy( std::string s )
{
  ltrim( s );
  return s;
}

inline std::string rtrim_copy( std::string s )
{
  rtrim( s );
  return s;
}

inline std::string trim_copy( std::string s )
{
  trim( s );
  return s;
}

inline std::string binary_string_from_hex_string( const std::string& hex )
{
  std::string result = "";
  for ( auto i = 0u; i < hex.size(); ++i )
  {
    switch ( hex[i] )
    {
    case '0':
      result.append( "0000" );
      break;
    case '1':
      result.append( "0001" );
      break;
    case '2':
      result.append( "0010" );
      break;
    case '3':
      result.append( "0011" );
      break;
    case '4':
      result.append( "0100" );
      break;
    case '5':
      result.append( "0101" );
      break;
    case '6':
      result.append( "0110" );
      break;
    case '7':
      result.append( "0111" );
      break;
    case '8':
      result.append( "1000" );
      break;
    case '9':
      result.append( "1001" );
      break;
    case 'a':
      result.append( "1010" );
      break;
    case 'b':
      result.append( "1011" );
      break;
    case 'c':
      result.append( "1100" );
      break;
    case 'd':
      result.append( "1101" );
      break;
    case 'e':
      result.append( "1110" );
      break;
    case 'f':
      result.append( "1111" );
      break;
    }
  }
  std::reverse( result.begin(), result.end() );
  return result;
}

inline std::string hex_string_from_binary_string( const std::string& binary )
{
  assert( ( binary.size() % 4 ) == 0 );
  std::string r = "";
  for ( auto i = 0u; i < binary.size(); i += 4 )
  {
    const auto bucket = binary.substr( i, 4 );
    if ( bucket == "0000" )
      r += '0';
    else if ( bucket == "0001" )
      r += '1';
    else if ( bucket == "0010" )
      r += '2';
    else if ( bucket == "0011" )
      r += '3';
    else if ( bucket == "0100" )
      r += '4';
    else if ( bucket == "0101" )
      r += '5';
    else if ( bucket == "0110" )
      r += '6';
    else if ( bucket == "0111" )
      r += '7';
    else if ( bucket == "1000" )
      r += '8';
    else if ( bucket == "1001" )
      r += '9';
    else if ( bucket == "1010" )
      r += 'a';
    else if ( bucket == "1011" )
      r += 'b';
    else if ( bucket == "1100" )
      r += 'c';
    else if ( bucket == "1101" )
      r += 'd';
    else if ( bucket == "1110" )
      r += 'e';
    else if ( bucket == "1111" )
      r += 'f';
  }
  return r;
}

} // namespace easy::utils

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
