/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "detail/bit_matrix.hpp"
#include "dynamic_bitset.hpp"
#include "foreach.hpp"
#include "permute.hpp"

#include <cassert>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace tweedledum {

/*! \brief Row-major matrix of bits */
template<class WordType = uint32_t>
class bit_matrix_rm final {
public:
	using word_type = WordType;
	using row_type = dynamic_bitset<WordType>;
	using column_type = dynamic_bitset<WordType>;
	using size_type = std::size_t;
	using container_type = detail::bit_matrix<WordType>;

#pragma region Constructors
	bit_matrix_rm(size_type num_rows)
	    : storage_(num_rows)
	{}

	bit_matrix_rm(size_type num_rows, size_type num_columns)
	    : storage_(num_rows, num_columns)
	{}

	template<typename ValueType>
	bit_matrix_rm(size_type num_columns, std::vector<ValueType> const& rows)
	    : storage_(num_columns, rows)
	{}
#pragma endregion

#pragma region Element access
	constexpr auto at(size_type row_index, size_type column_index) const
	{
		return row(row_index)[column_index];
	}

	constexpr auto at(size_type row_index, size_type column_index)
	{
		return row(row_index)[column_index];
	}

	constexpr auto const& row(size_type index) const
	{
		return storage_.line(index);
	}

	constexpr auto& row(size_type index)
	{
		return storage_.line(index);
	}

	constexpr auto column(size_type column_index) const
	{
		column_type value(num_rows(), 0);
		for (size_type row_index = 0; row_index < num_rows(); ++row_index) {
			value[row_index] = row(row_index)[column_index];
		}
		return value;
	}
#pragma endregion

#pragma region Iterators
	// TODO
	template<typename Fn>
	void foreach_column(Fn&& fn) const;

	// TODO
	template<typename Fn>
	void foreach_column(Fn&& fn);

	template<typename Fn>
	void foreach_row(Fn&& fn) const
	{
		foreach_element(storage_.lines_.cbegin(), storage_.lines_.cend(), fn, 0);
	}

	template<typename Fn>
	void foreach_row(Fn&& fn)
	{
		foreach_element(storage_.lines_.begin(), storage_.lines_.end(), fn, 0);
	}
#pragma endregion

#pragma region Capacity
	constexpr auto size() const
	{
		return std::make_pair(num_rows(), num_columns());
	}

	constexpr auto num_rows() const
	{
		return storage_.num_lines();
	}

	constexpr auto num_columns() const
	{
		return storage_.num_bits_per_line();
	}

	constexpr auto empty() const
	{
		return storage_.empty();
	}
#pragma endregion

#pragma region Properties
	constexpr auto is_square() const
	{
		return storage_.is_square();
	}

	constexpr auto is_identity() const
	{
		return storage_.is_identity();
	}
#pragma endregion

#pragma region Modifiers
	constexpr void push_back_row(row_type const& row)
	{
		assert(row.size() == num_columns());
		storage_.push_back_line(row);
	}

	void transpose()
	{
		storage_.transpose();
	}

	auto permute_rows(std::vector<uint32_t> const& rows) const
	{
		assert(rows.size() == num_rows());
		bit_matrix_rm permuted_matrix(num_columns());
		for (auto row : rows) {
			permuted_matrix.push_back_row(this->row(row));
		}
		return permuted_matrix;
	}

	void permute_rows(std::vector<uint32_t> const& rows)
	{
		assert(rows.size() == num_rows());
		apply_permutation(storage_.lines_, rows);
	}
#pragma endregion

#pragma region Debug
	void print(std::ostream& out = std::cout) const
	{
		for (size_type row_index = 0; row_index < num_rows(); ++row_index) {
			auto row = this->row(row_index);
			for (auto bit : row) {
				out << fmt::format("{} ", int(bit));
			}
			out << fmt::format("\n");
		}
	}
#pragma endregion
private:
	container_type storage_;
};

} // namespace tweedledum
