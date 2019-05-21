/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../dynamic_bitset.hpp"

#include <cassert>
#include <cstdlib>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace tweedledum::detail {

template<class WordType = uint32_t>
struct bit_matrix {
	static_assert(std::is_integral<WordType>::value, "Integral type required.");
	static_assert(!std::is_same<WordType, bool>::value, "Not bool though (:");

	using word_type = WordType;
	using line_type = dynamic_bitset<word_type>;
	using size_type = std::size_t;
	using container_type = std::vector<line_type>;

#pragma region Constructors
	bit_matrix(size_type num_bits_per_line)
	    : num_bits_per_line_(num_bits_per_line)
	    , lines_()
	{}

	bit_matrix(size_type num_bits_per_line, size_type num_lines)
	    : num_bits_per_line_(num_bits_per_line)
	    , lines_(num_lines, line_type(num_bits_per_line, false))
	{}

	template<typename ValueType>
	bit_matrix(size_type num_bits_per_line, std::vector<ValueType> const& words)
	    : num_bits_per_line_(0)
	    , lines_()
	{
		init_from_value_vector(num_bits_per_line, words);
	}
#pragma endregion

#pragma region Element access
	constexpr auto const& line(size_type index) const
	{
		assert(index < lines_.size());
		return lines_[index];
	}

	constexpr auto& line(size_type index)
	{
		assert(index < lines_.size());
		return lines_[index];
	}
#pragma endregion

#pragma region Capacity
	constexpr auto num_bits_per_line() const
	{
		return num_bits_per_line_;
	}

	constexpr auto num_lines() const
	{
		return lines_.size();
	}

	constexpr auto empty() const
	{
		return lines_.empty() || num_bits_per_line_ == 0;
	}
#pragma endregion

#pragma region Properties
	constexpr auto is_square() const
	{
		return num_bits_per_line_ == lines_.size();
	}

	constexpr auto is_identity() const
	{
		if (!is_square()) {
			return false;
		}
		for (auto i = 0; i < lines_.size(); ++i) {
			if (lines_[i][i] != 1) {
				return false;
			}
			if (lines_[i].count() > 1) {
				return false;
			}
		}
		return true;
	}
#pragma endregion

#pragma region Modifiers
	constexpr void push_back_line(dynamic_bitset<WordType> const& line)
	{
		assert(line.size() == num_bits_per_line_);
		lines_.emplace_back(line);
	}

	template <typename T>
	constexpr void emplace_back_line(T&& line)
	{
		lines_.emplace_back(num_bits_per_line_, std::forward<T>(line));
	}

	// This method modify the matrix directly
	// I wonder if it wouldn't be better to return a new transposed version of
	// this matrix
	void transpose()
	{
		// Take a faster path
		if (num_bits_per_line_ == lines_.size()) {
			square_transpose();
			return;
		}
		// Slow path
		// TODO: impelment other cases
		assert(0 && "Transpose method not implemented for the general case");
		std::abort();
	}
#pragma endregion

	size_type num_bits_per_line_;
	container_type lines_;

private:
	template<typename ValueType>
	void init_from_value_vector(size_type num_rows, std::vector<ValueType> const& lines_values)
	{
		static_assert(std::is_integral<ValueType>::value, "Integral type required.");
		assert(lines_.size() == 0);
		num_bits_per_line_ = num_rows;
		for (auto line_value : lines_values) {
			lines_.emplace_back(num_bits_per_line_, line_value);
		}
	}

	void square_transpose()
	{
		for (auto j = 1u; j < num_bits_per_line_; ++j) {
			for (auto i = 0u; i < j; ++i) {
				if (lines_[i][j] == lines_[j][i]) {
					continue;
				}
				lines_[i][j] ^= 1;
				lines_[j][i] ^= 1;
			}
		}
	}
};

} // namespace tweedledum::detail
