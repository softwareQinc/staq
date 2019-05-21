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

namespace alice
{

class ec_command : public command
{
public:
  explicit ec_command( const environment::ptr& env )
      : command( env, "check equivalence of two ESOPs" )
  {
    opts.add_option( "store index", i, "First index in ESOP storage (default: 0)" );
    opts.add_option( "store index", j, "Second index in ESOP storage (default: 0)" );
    opts.add_flag( "--func,-f", func, "Check if an ESOP implements a Boolean function. If set, "
                                      "the first index identifies the function in the store." );
  }

protected:
  rules validity_rules() const
  {
    rules rules;

    if ( !func )
    {
      rules.push_back( {[this]() { return i < store<esop_storee>().size(); }, "first index out of bounds"} );
    }
    else
    {
      rules.push_back( {[this]() { return i < store<function_storee>().size(); }, "first index out of bounds"} );
    }
    rules.push_back( {[this]() { return j < store<esop_storee>().size(); }, "second index out of bounds"} );

    return rules;
  }

  void execute()
  {
    if ( !func )
    {
      /* check if two ESOPs are equivalent */
      const auto& elm1 = store<esop_storee>()[i];
      const auto& elm2 = store<esop_storee>()[j];

      if ( elm1.number_of_inputs != elm2.number_of_inputs )
      {
        std::cout << "[i] ESOPs are NOT described using the same number of inputs" << std::endl;
        return;
      }

      if ( easy::esop::equivalent_esops( elm1.esop, elm2.esop, elm1.number_of_inputs ) )
      {
        std::cout << "[i] ESOPs are equivalent" << std::endl;
      }
      else
      {
        std::cout << "[i] ESOPs are NOT equivalent" << std::endl;
      }
    }
    else
    {
      /* check if an ESOP implements a Boolean function */
      const auto& func = store<function_storee>()[i];
      const auto& elm = store<esop_storee>()[j];

      if ( elm.number_of_inputs != func.number_of_variables )
      {
        std::cout << "[i] ESOP and Boolean function are NOT described using the same number of variables" << std::endl;
        return;
      }

      if ( easy::esop::implements_function( elm.esop, func.bits, func.care, elm.number_of_inputs ) )
      {
        std::cout << "[i] ESOP is an implementation of the Boolean function" << std::endl;
      }
      else
      {
        std::cout << "[i] ESOP does NOT implement the Boolean function" << std::endl;
      }
    }
  }

private:
  bool func = false;
  uint32_t i = 0;
  uint32_t j = 0;
}; /* ec_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
