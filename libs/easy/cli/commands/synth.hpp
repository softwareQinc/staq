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

namespace alice
{

class synth_command : public command
{
public:
  explicit synth_command( const environment::ptr& env )
      : command( env, "SAT-based synthesis of ESOP forms" )
  {
    opts.add_option( "--terms,-t", number_of_terms, "Maximum number of terms used for synthesis (default: 10)" );
    opts.add_option( "--conflicts,-c", number_of_conflicts, "Maximum number of conflicts (default: 10000)" );
    opts.add_option( "--strategy,-s", strategy, "Synthesis strategy (default: 0)\n"
                                                "\tfixed-size 0\n"
                                                "\tdownward 1\n"
                                                "\tupward 2\n" );
    opts.add_flag( "--all,-a", all_flag, "Use all functions in the function store" );
    opts.add_flag( "--delete,-d", delete_flag, "Do not store any result but delete them" );
  }

protected:
  void execute()
  {
    auto number_of_realizable = 0u;
    auto number_of_unrealizable = 0u;
    auto number_of_unknown = 0u;
    auto total_num_terms = 0;

    auto counter = 0;
    auto total_duration = 0.0;

    const auto function_store_size = store<function_storee>().size();
    if ( !all_flag )
    {
      ++counter;

      const auto& func = store<function_storee>()[function_store_size - 1u];

      auto bits = to_binary( func.bits );
      auto care = to_binary( func.care );
      std::reverse( bits.begin(), bits.end() );
      std::reverse( care.begin(), care.end() );

      const auto start_time = std::chrono::system_clock::now();

      easy::esop::result synthesis_result;
      if ( strategy == 0 )
      {
        easy::esop::simple_synthesizer_params params;
        params.conflict_limit = number_of_conflicts;
        params.number_of_terms = number_of_terms;

        easy::esop::simple_synthesizer synthesizer( easy::esop::spec{bits, care} );
        synthesis_result = synthesizer.synthesize( params );
      }
      else if ( strategy == 1 )
      {
        easy::esop::minimum_synthesizer_params params;
        params.conflict_limit = number_of_conflicts;
        params.begin = number_of_terms;
        params.next = [&]( uint32_t& i, easy::sat::sat_solver::result sat ) { if ( i <= 1 || sat.is_unsat() ) return false; --i; return true; };

        easy::esop::minimum_synthesizer synthesizer( easy::esop::spec{bits, care} );
        synthesis_result = synthesizer.synthesize( params );
      }
      else if ( strategy == 2 )
      {
        easy::esop::minimum_synthesizer_params params;
        params.conflict_limit = number_of_conflicts;
        params.begin = 1;
        params.next = [&]( uint32_t& i, easy::sat::sat_solver::result sat ) { if ( i >= number_of_terms || sat.is_sat() ) return false; ++i; return true; };

        easy::esop::minimum_synthesizer synthesizer( easy::esop::spec{bits, care} );
        synthesis_result = synthesizer.synthesize( params );
      }
      else
      {
        std::cout << "[e] unknown strategy" << std::endl;
        return;
      }

      const auto end_time = std::chrono::system_clock::now();
      total_duration += ( std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time ).count() / 1000.0 );

      if ( synthesis_result.is_unknown() )
      {
        std::cout << "[w] could not synthesize function (conflict limit too tight)" << std::endl;
        ++number_of_unknown;
      }
      else if ( synthesis_result.is_unrealizable() )
      {
        std::cout << "[i] function is unrealizable" << std::endl;
        ++number_of_unrealizable;
      }
      else
      {
        assert( synthesis_result.is_realizable() );
        ++number_of_realizable;
        total_num_terms += synthesis_result.esop.size();
        if ( !delete_flag )
          env->store<esop_storee>().extend() = esop_storee{"", synthesis_result.esop, func.number_of_variables, 1};
      }
    }
    else
    {
      for ( auto i = 0u; i < function_store_size; ++i )
      {
        ++counter;

        const auto& func = store<function_storee>()[i];

        auto bits = to_binary( func.bits );
        auto care = to_binary( func.care );
        std::reverse( bits.begin(), bits.end() );
        std::reverse( care.begin(), care.end() );

        const auto start_time = std::chrono::system_clock::now();

        easy::esop::result synthesis_result;
        if ( strategy == 0 )
        {
          easy::esop::simple_synthesizer_params params;
          params.conflict_limit = number_of_conflicts;
          params.number_of_terms = number_of_terms;

          easy::esop::simple_synthesizer synthesizer( easy::esop::spec{bits, care} );
          synthesis_result = synthesizer.synthesize( params );
        }
        else if ( strategy == 1 )
        {
          easy::esop::minimum_synthesizer_params params;
          params.conflict_limit = number_of_conflicts;
          params.begin = number_of_terms;
          params.next = [&]( uint32_t& i, easy::sat::sat_solver::result sat ) { if ( i <= 1 || sat.is_unsat() ) return false; --i; return true; };

          easy::esop::minimum_synthesizer synthesizer( easy::esop::spec{bits, care} );
          synthesis_result = synthesizer.synthesize( params );
        }
        else if ( strategy == 2 )
        {
          easy::esop::minimum_synthesizer_params params;
          params.conflict_limit = number_of_conflicts;
          params.begin = 1;
          params.next = [&]( uint32_t& i, easy::sat::sat_solver::result sat ) { if ( i >= number_of_terms || sat.is_sat() ) return false; ++i; return true; };

          easy::esop::minimum_synthesizer synthesizer( easy::esop::spec{bits, care} );
          synthesis_result = synthesizer.synthesize( params );
        }
        else
        {
          std::cout << "[e] unknown strategy" << std::endl;
          return;
        }

        const auto end_time = std::chrono::system_clock::now();
        total_duration += ( std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time ).count() / 1000.0 );

        if ( synthesis_result.is_unknown() )
        {
          std::cout << "[w] could not synthesize function (conflict limit too tight) " << i << std::endl;
          ++number_of_unknown;
        }
        else if ( synthesis_result.is_unrealizable() )
        {
          std::cout << "[i] function is unrealizable" << std::endl;
          ++number_of_unrealizable;
        }
        else
        {
          assert( synthesis_result.is_realizable() );
          ++number_of_realizable;
          total_num_terms += synthesis_result.esop.size();
          if ( !delete_flag )
            env->store<esop_storee>().extend() = esop_storee{"", synthesis_result.esop, func.number_of_variables, 1};
        }
      }
    }

    std::cout << "[i] " << rang::style::bold << "results: " << rang::style::reset;
    std::cout << fmt::format( "#realizable={} / #unrealizable={} / #unknown={} / avg number of terms={}\n",
                              number_of_realizable, number_of_unrealizable, number_of_unknown, ( double( total_num_terms ) / number_of_realizable ) );
    std::cout << "[i] " << rang::style::bold << "time: " << rang::style::reset;
    std::cout << fmt::format( "total={}s / avg per func={}s\n",
                              total_duration, ( total_duration / counter ) );
  }

private:
  unsigned number_of_terms = 8u;
  unsigned number_of_conflicts = 10000u;
  bool all_flag = false;
  bool delete_flag = false;
  int strategy = 0;
}; /* synth_command */

} // namespace alice

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
