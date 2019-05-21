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
#include <algorithm>

namespace alice
{

class sort_command : public command
{
public:
  explicit sort_command( const environment::ptr& env )
      : command( env, "sort current ESOP" )
  {
    opts.add_flag( "-i,--index", index, "Index" );
    opts.add_flag( "-r,--random", random, "Randomize the cubes in the ESOP" );
  }

protected:
  rules validity_rules() const
  {
    rules rules;

    rules.push_back( {[this]() { return index == -1 || uint32_t(index) < store<esop_storee>().size(); }, "index out of bounds"} );

    return rules;
  }

  void execute()
  {
    std::random_device rd;
    std::mt19937 g( rd() );

    auto& current = index == -1 ? store<esop_storee>().current() : store<esop_storee>()[index];
    if ( random )
    {
      std::shuffle( current.esop.begin(), current.esop.end(), g );
    }
    else
    {
      std::sort( current.esop.begin(), current.esop.end() );
    }
  }

private:
  bool random;
  int index = -1;
}; /* sort_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
