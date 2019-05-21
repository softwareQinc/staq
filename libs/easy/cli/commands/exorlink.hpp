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

#include <easy/esop/exorlink.hpp>

namespace alice
{

std::uint32_t k_size[] = {/* 0 */ 0, /* 1 */ 0, /* 2 */ 8, /* 3 */ 54, /* 4 */ 384, /* 5 */ 3000, /* 6 */ 25920};
std::uint32_t k_incr[] = {/* 0 */ 0, /* 1 */ 0, /* 2 */ 4, /* 3 */ 9, /* 4 */ 16, /* 5 */ 25, /* 6 */ 36};

std::uint32_t* cube_groups[] = {
    nullptr, nullptr, &easy::esop::cube_groups2[0], &easy::esop::cube_groups3[0], &easy::esop::cube_groups4[0], &easy::esop::cube_groups5[0], &easy::esop::cube_groups6[0]};

class exorlink_command : public command
{
public:
  explicit exorlink_command( const environment::ptr& env )
      : command( env, "exorlink operation" )
  {
    opts.add_option( "cube0", cube0, "First cube" )->required();
    opts.add_option( "cube1", cube1, "Second cube" )->required();
    opts.add_flag( "-r,--reverse", change_order, "Reverse variable order (default: 0...n left to right)" );
  }

protected:
  void execute()
  {
    const auto num_vars = cube0.size();
    if ( cube1.size() != num_vars )
    {
      std::cerr << "[e] cube must have the same length" << std::endl;
      return;
    }

    if ( !change_order )
    {
      std::reverse( cube0.begin(), cube0.end() );
      std::reverse( cube1.begin(), cube1.end() );
    }

    const auto c0 = kitty::cube( cube0 );
    const auto c1 = kitty::cube( cube1 );

    const auto d = c0.distance( c1 );
    std::cout << "[i] distance = " << d << std::endl;

    if ( d >= 2 && d <= 7 )
    {
      const auto size = k_size[d];
      const auto incr = k_incr[d];
      for ( auto k = 0u; k < size; k += incr )
      {
        const auto cubes = easy::esop::exorlink( cube0, cube1, d, cube_groups[d] + k );
        for ( const auto& c : cubes )
        {
          c.print( num_vars );
          std::cout << ' ';
        }
        std::cout << std::endl;
      }
    }
    else
    {
      std::cerr << "[e] only distances in the interval [2,...,6] are supported" << std::endl;
    }
  }

private:
  std::string cube0;
  std::string cube1;
  bool change_order = true;
}; /* exorlink_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
