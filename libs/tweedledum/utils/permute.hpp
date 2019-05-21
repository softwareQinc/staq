/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <algorithm>
#include <utility>
#include <vector>

namespace tweedledum {

template<typename T>
void apply_permutation(std::vector<T>& v, std::vector<uint32_t> const& indices)
{
	std::vector<T> v2;
	v2.reserve(v.size());
	for (auto i = 0ull; i < v.size(); ++i) {
		v2.push_back(std::move(v[indices[i]]));
	}
	v = std::move(v2);
}

inline auto permutation_to_transpositions(std::vector<uint32_t> permutation)
{
	std::vector<std::pair<uint32_t, uint32_t>> transpositions;

	for (auto i = permutation.size(); i-- > 0;) {
		if (permutation[i] == i) {
			continue;
		}
		const auto element_it = std::find(permutation.begin(), permutation.begin() + i, i);
		const auto index = std::distance(permutation.begin(), element_it);
		transpositions.push_back(std::make_pair(i, index));
		std::swap(permutation[i], permutation[index]);
	}
	std::reverse(transpositions.begin(), transpositions.end());
	return transpositions;
}

} // namespace tweedledum
