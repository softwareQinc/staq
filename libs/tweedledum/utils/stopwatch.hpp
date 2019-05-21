/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>

namespace tweedledum {

/*! \brief Stopwatch interface
 *
 * This class implements a stopwatch interface to track time.  It starts
 * tracking time at construction and stops tracking time at deletion
 * automatically.  A reference to a duration object is passed to the
 * constructor.  After stopping the time the measured time interval is added
 * to the durationr reference.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration_type time = 0;
      { // some block
        stopwatch t(time);
        // do some work
      } // stopwatch is stopped here
      std::cout << fmt::format("{:5.2f} seconds passed\n", to_seconds(time));
   \endverbatim
 */
template<class Clock = std::chrono::steady_clock>
class stopwatch {
public:
	using clock = Clock;
	using duration_type = typename Clock::duration;
	using time_point_type = typename Clock::time_point;

	/*! \brief Default constructor.
	 *
	 * Starts tracking time.
	 */
	explicit stopwatch(duration_type& duration)
	    : duration(duration)
	    , begin(clock::now())
	{}


	/*! \brief .
	 *
	 * Stops tracking time and updates duration.
	 */
	void stop()
	{
		duration += (clock::now() - begin);
	}

	/*! \brief Default deconstructor.
	 *
	 * Stops tracking time and updates duration.
	 */
	~stopwatch()
	{
		duration += (clock::now() - begin);
	}

private:
	duration_type& duration;
	time_point_type begin;
};

/*! \brief Calls a function and tracks time.
 *
 * The function that is passed as second parameter can be any callable object
 * that takes no parameters.  This construction can be used to avoid
 * pre-declaring the result type of a computation that should be tracked.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration_type time = 0;
      auto result = call_with_stopwatch(time, [&]() { return function(parameters); });
   \endverbatim
 *
 * \param duration Duration reference (time will be added to it)
 * \param fn Callable object with no arguments
 */
template<class Fn, class Clock = std::chrono::steady_clock>
inline std::invoke_result_t<Fn> call_with_stopwatch(typename Clock::duration& duration, Fn&& fn)
{
	stopwatch<Clock> t(duration);
	return fn();
}

/*! \brief Constructs an object and calls time.
 *
 * This function can track the time for the construction of an object and
 * returns the constructed object.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration_type time = 0;
      // create vector with 100000 elements initialized to 42
      auto result = make_with_stopwatch<std::vector<int>>(time, 100000, 42);
   \endverbatim
 */
template<class T, class... Args, class Clock = std::chrono::steady_clock>
inline T make_with_stopwatch(typename Clock::duration& duration, Args... args)
{
	stopwatch<Clock> t(duration);
	return T{std::forward<Args>(args)...};
}

/*! \brief Utility function to convert duration into seconds. */
template<class Duration>
inline double to_seconds(Duration const& duration)
{
	return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}

} // namespace tweedledum