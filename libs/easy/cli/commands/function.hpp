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
#include <random>
#include <chrono>

namespace alice
{

class function_command : public command
{
public:
  explicit function_command( const environment::ptr& env )
      : command( env, "loads an incompletely-specified Boolean function and adds it to the store" )
  {
    opts.add_option( "--num,-n", num, "Specifies how many Boolean functions are generated (default: 1)" );
    opts.add_option( "--tt,-t", tt, "Truth table (in binary) of Boolean function (MSB ... LSB)" );
    opts.add_option( "--random,-r", variables, "Random completely-specified Boolean function.  As parameter the number of Boolean variables has to be passed." );
    opts.add_option( "--seed,-s", seed, "Random seed" );
  }

protected:
  void execute()
  {
    if ( tt != "" )
    {
      const unsigned number_of_variables = std::ceil( std::log2( tt.size() ) );

      std::reverse( tt.begin(), tt.end() );

      kitty::dynamic_truth_table bits( number_of_variables );
      kitty::dynamic_truth_table care( number_of_variables );

      unsigned i;
      for ( i = 0; i < tt.size(); ++i )
      {
        if ( tt[i] != '1' && tt[i] != '0' && tt[i] != '-' )
        {
          std::cout << fmt::format( "[w] unsupported character `{}` treated as don't care\n", tt[i] );
        }

        if ( tt[i] == '1' )
        {
          kitty::set_bit( bits, i );
          kitty::set_bit( care, i );
        }
        else if ( tt[i] == '0' )
        {
          kitty::clear_bit( bits, i );
          kitty::set_bit( care, i );
        }
      }

      for ( auto j = 0u; j < num; ++j )
      {
        env->store<function_storee>().extend() = function_storee{bits, care, number_of_variables};
      }
    }
    else
    {
      assert( variables != 0 );

      std::default_random_engine random_engine( seed != 0 ? seed : std::chrono::steady_clock::now().time_since_epoch().count() );
      std::uniform_int_distribution<> dist( 0, 100 );

      const auto len = 1ul << variables;
      for ( auto j = 0u; j < num; ++j )
      {
        kitty::dynamic_truth_table bits( variables );
        kitty::dynamic_truth_table care( variables );

        /* bits */
        for ( auto i = 0ul; i < len; ++i )
        {
          if ( dist( random_engine ) > 50 )
          {
            set_bit( bits, i );
          }
          else
          {
            clear_bit( bits, i );
          }
        }

        /* care */
        for ( auto i = 0ul; i < len; ++i )
        {
          if ( dist( random_engine ) > 50 )
          {
            set_bit( care, i );
          }
          else
          {
            clear_bit( care, i );
          }
        }

        env->store<function_storee>().extend() = function_storee{bits, care, variables};
      }
    }
  }

private:
  unsigned num = 1u;
  std::string tt = "";
  unsigned variables = 0u;
  unsigned long seed = 0ul;
}; /* function_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
