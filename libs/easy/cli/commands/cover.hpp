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
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/bit_operations.hpp>
#include <easy/esop/synthesis.hpp>
#include <easy/esop/esop.hpp>

namespace alice
{

class cover_command : public command
{
public:
  explicit cover_command( const environment::ptr& env )
      : command( env, "computes the cover of an incompletely-specified Boolean function and adds it to the ESOP store" )
  {
    opts.add_option( "store index", i, "Index in function storage (default: last element)" );
  }

protected:
  rules validity_rules() const
  {
    rules rules;

    rules.push_back( {[this]() { return uint32_t(i) < store<function_storee>().size() || ( i == -1 && store<function_storee>().size() > 0 ); }, "first index out of bounds"} );

    return rules;
  }

  void execute()
  {
    const auto& elm = i == -1 ? store<function_storee>()[store<function_storee>().size() - 1u] : store<function_storee>()[i];

    easy::esop::spec spec;
    assert( elm.bits.num_bits() == elm.care.num_bits() );
    for ( auto i = 0u; i < elm.bits.num_bits(); ++i )
    {
      spec.bits.append( 1, kitty::get_bit( elm.bits, i ) ? '1' : '0' );
      spec.care.append( 1, kitty::get_bit( elm.care, i ) ? '1' : '0' );
    }

    std::reverse( spec.bits.begin(), spec.bits.end() );
    std::reverse( spec.care.begin(), spec.care.end() );

    env->store<esop_storee>().extend() = {"", easy::esop::esop_cover( spec ), std::size_t( elm.bits.num_vars() ), 1};
  }

private:
  int i = -1;
}; /* cover_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
