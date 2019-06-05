/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../networks/io_id.hpp"

#include <string>

namespace tweedledum {

/*! \brief Creates a new network with same I/O's as the original.
 *
 * This function requires a template parameter that cannot be inferred. Useful when duplicate into a
 * different network type.
 * 
 * **Required network functions:**
 * - `add_cbit`
 * - `add_qubit`
 * - `foreach_io`
 * - `name`
 *
 * \param original The original network.
 * \param name The name of the new network (default: same as the original).
 */
template<class NetworkOriginal, class Network>
Network shallow_duplicate(NetworkOriginal const& original, std::string_view name = {})
{
	Network duplicate(name.empty() ? original.name() : name);
	original.foreach_io([&](io_id io, std::string const& label) {
		if (io.is_qubit()) {
			duplicate.add_qubit(label);
		} else {
			duplicate.add_cbit(label);
		}
	});
	return duplicate;
}

/*! \brief Creates a new network with same I/O's as the original.
 *
 * **Required network functions:**
 * - `add_cbit`
 * - `add_qubit`
 * - `foreach_io`
 * - `name`
 *
 * \param original The original network.
 * \param name The name of the new network (default: same as the original).
 */
template<class Network>
Network shallow_duplicate(Network const& original, std::string_view name = {})
{
	Network duplicate(name.empty() ? original.name() : name);
	original.foreach_io([&](io_id io, std::string const& label) {
		if (io.is_qubit()) {
			duplicate.add_qubit(label);
		} else {
			duplicate.add_cbit(label);
		}
	});
	return duplicate;
}

} // namespace tweedledum
