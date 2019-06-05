/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "angle.hpp"

#include <cassert>
#include <fmt/format.h>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace tweedledum {

/*! \brief
 */
template<typename Term = uint32_t>
class parity_terms {
public:
#pragma region Types and constructors
	parity_terms() = default;
#pragma endregion

#pragma region Properties
	auto num_terms() const
	{
		return term_to_angle_.size();
	}
#pragma endregion

#pragma region Iterators
	auto begin() const
	{
		return term_to_angle_.cbegin();
	}

	auto end() const
	{
		return term_to_angle_.cend();
	}
#pragma endregion

#pragma region Modifiers
	/*! \brief Add parity term.
	 *
	 * If the term already exist it increments the rotation angle
	 */
	void add_term(Term term, angle rotation_angle)
	{
		assert(rotation_angle != angles::zero);
		auto search = term_to_angle_.find(term);
		if (search != term_to_angle_.end()) {
			search->second += rotation_angle;
		} else {
			term_to_angle_.emplace(term, rotation_angle);
		}
	}

	/*! \brief Extract parity term. */
	angle extract_term(Term term)
	{
		auto search = term_to_angle_.find(term);
		if (search != term_to_angle_.end()) {
			term_to_angle_.erase(search);
			return search->second;
		} else {
			return angles::zero;
		}
	}
#pragma endregion

private:
	std::unordered_map<Term, angle> term_to_angle_;
};

} // namespace tweedledum
