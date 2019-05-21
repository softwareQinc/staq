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

#include <alice/alice.hpp>
#include <iostream>


/* https://stackoverflow.com/questions/21438631/fastest-way-of-computing-the-power-that-a-power-of-2-number-used */
int ilog2( int value )
{
  static constexpr int twos[] = {
    1<<0,  1<<1,  1<<2,  1<<3,  1<<4,  1<<5,  1<<6,  1<<7,
    1<<8,  1<<9,  1<<10, 1<<11, 1<<12, 1<<13, 1<<14, 1<<15,
    1<<16, 1<<17, 1<<18, 1<<19, 1<<20, 1<<21, 1<<22, 1<<23,
    1<<24, 1<<25, 1<<26, 1<<27, 1<<28, 1<<29, 1<<30, 1<<31
  };

  return std::lower_bound( std::begin( twos ), std::end( twos ), value ) - std::begin( twos );
}

namespace alice
{

class read_fns_command : public command
{
public:
  explicit read_fns_command( const environment::ptr& env )
      : command( env, "read functions from file" )
  {
    opts.add_option( "filename", filename, "File to read" );
  }

protected:
  rules validity_rules() const
  {
    rules rules;
    rules.push_back( {[this]() { return filename != ""; }, "no filename specified"} );
    return rules;
  }

  void execute()
  {
    env->out() << "[i] read functions from file " << filename << std::endl;

    std::ifstream ifs( filename );
    std::string line;
    while ( std::getline( ifs, line ) )
    {
      // 16*4 == 64 bit == 2^6
      auto const num_vars = ilog2( 4u*( line.size() - 2u ) );
      assert( num_vars > 0 );
      store<function_storee>().extend() = function_storee::from_hex_string( line, num_vars );
    }
    ifs.close();
  }

protected:
  std::string filename = "";
}; /* read_fns_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
