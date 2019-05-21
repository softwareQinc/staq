/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <cassert>
#include <memory>
#include <type_traits>
#include <vector>

namespace tweedledum {

/*! Bitset in which number of variables is known at runtime. */
template<class WordType = uint32_t>
class dynamic_bitset {
	static_assert(std::is_integral<WordType>::value, "Integral type required.");
	static_assert(!std::is_same<WordType, bool>::value, "Not bool though (:");
	using container_type = std::vector<WordType>;

public:
	using block_type = WordType ;
	using size_type = std::size_t ;
	using block_width_type = typename container_type::size_type ;

	static constexpr block_width_type block_width = std::numeric_limits<block_type>::digits;
	static constexpr size_type npos = static_cast<size_type>(-1);

public:
	using const_reference = bool;
	class reference {
		friend class dynamic_bitset<block_type>;

		reference(block_type& block, block_width_type position)
		    : block_(block)
		    , mask_((assert(position < block_width), block_type(1) << position))
		{}

		void operator&();

	public:
		operator bool() const
		{
			return (block_ & mask_) != 0;
		}

		bool operator~() const
		{
			return (block_ & mask_) == 0;
		}

		reference& flip()
		{
			block_ ^= mask_;
			return *this;
		}

		reference& operator=(bool value)
		{
			value ? block_ |= mask_ : block_ &= ~mask_;
			return *this;
		}

		reference& operator=(reference const& rhs)
		{
			rhs ? block_ |= mask_ : block_ &= ~mask_;
			return *this;
		}

		reference& operator|=(bool value)
		{
			if (value) {
				block_ |= mask_;
			}
			return *this;
		}

		reference& operator&=(bool value)
		{
			if (!value) {
				block_ &= ~mask_;
			}
			return *this;
		}

		reference& operator^=(bool value)
		{
			if (value) {
				block_ ^= mask_;
			}
			return *this;
		}

		reference& operator-=(bool value)
		{
			if (value) {
				block_ &= ~mask_;
			}
			return *this;
		}

	private:
		block_type& block_;
		const block_type mask_;
	};

	class iterator {
		friend class dynamic_bitset<block_type>;

		iterator(typename container_type::iterator block, block_width_type position)
		    : current_block_(block)
		    , position_((assert(position < block_width), position))
		{
			// std::cout << fmt::format("{}\n", remaining_);
		}

	public:
		// Iterator traits, previously from std::iterator.
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;

		// Default constructible.
		iterator() = default;

		// Dereferencable.
		reference operator*() const
		{
			return reference(*current_block_, position_);
		}

		// Pre- and post-incrementable.
		constexpr iterator& operator++()
		{
			if (position_ + 1 < block_width) {
				++position_;
			} else {
				++current_block_;
				position_ = 0;
			}
			return *this;
		}

		constexpr iterator operator++(int)
		{
			iterator old = *this;
			++(*this);
			return old;
		}

		// Pre- and post-decrementable.
		constexpr iterator& operator--()
		{
			if (position_) {
				--position_;
			} else {
				--current_block_;
				position_ = block_width - 1;
			}
			return *this;
		}

		constexpr iterator operator--(int)
		{
			iterator old = *this;
			--(*this);
			return old;
		}

		// Equality / inequality.
		constexpr bool operator==(const iterator& rhs)
		{
			return current_block_ == rhs.current_block_ && position_ == rhs.position_;
		}

		constexpr bool operator!=(const iterator& rhs)
		{
			return current_block_ != rhs.current_block_ || position_ != rhs.position_;
		}

	private:
		typename container_type::iterator current_block_;
		block_width_type position_;
	};

#pragma region Constructors
	constexpr dynamic_bitset() noexcept = default;

	template<typename ValueType>
	dynamic_bitset(size_type num_bits, ValueType value)
	    : num_bits_(0)
	    , bits_()
	{
		init_from_value(num_bits, value);
	}

	dynamic_bitset(size_type num_bits, bool value = false)
	    : num_bits_(num_bits)
	    , bits_(calculate_num_blocks(num_bits), value? ~block_type(0) : block_type(0))
	{
		zero_unused_bits();
	}

	dynamic_bitset(dynamic_bitset const& other)
	    : num_bits_(other.num_bits_)
	    , bits_(other.bits_)
	{}

	dynamic_bitset(dynamic_bitset&& other)
	    : num_bits_(std::move(other.num_bits_))
	    , bits_(std::move(other.bits_))
	{
		assert((other.bits_ = container_type()).empty());
		other.num_bits_ = 0;
	}
#pragma endregion

#pragma region Comparison
	bool operator==(dynamic_bitset const& rhs) const noexcept
	{
		return (num_bits_ == rhs.num_bits_) && (bits_ == rhs.bits_);
	}

	bool operator!=(dynamic_bitset const& rhs) const noexcept
	{
		return !(*this == rhs);
	}
#pragma endregion

#pragma region Dynamic bitset operations
	dynamic_bitset& operator&=(const dynamic_bitset& rhs) noexcept
	{
		assert(size() == rhs.size());
		for (size_type i = 0; i < num_blocks(); ++i) {
			bits_[i] &= rhs.bits_[i];
		}
		return *this;
	}

	dynamic_bitset& operator|=(const dynamic_bitset& rhs) noexcept
	{
		assert(size() == rhs.size());
		for (size_type i = 0; i < num_blocks(); ++i) {
			bits_[i] |= rhs.bits_[i];
		}
		return *this;
	}

	dynamic_bitset& operator^=(const dynamic_bitset& rhs) noexcept
	{
		assert(size() == rhs.size());
		for (size_type i = 0; i < this->num_blocks(); ++i) {
			bits_[i] ^= rhs.bits_[i];
		}
		return *this;
	}

	// TODO
	dynamic_bitset& operator<<=(size_type position) noexcept;
	dynamic_bitset& operator>>=(size_type position) noexcept;
#pragma endregion

#pragma region Bit operations
	dynamic_bitset& set() noexcept
	{
		std::fill(bits_.begin(), bits_.end(), static_cast<block_type>(~0));
		zero_unused_bits();
		return *this;
	}

	dynamic_bitset& set(size_type position, bool value = true)
	{
		assert(position < num_bits_);
		if (value) {
			bits_[block_index(position)] |= bit_mask(position);
		} else {
			reset(position);
		}
		return *this;
	}

	dynamic_bitset& reset() noexcept
	{
		std::fill(bits_.begin(), bits_.end(), block_type(0));
		return *this;
	}

	dynamic_bitset& reset(size_type position)
	{
		assert(position < num_bits_);
		bits_[block_index(position)] &= ~bit_mask(position);
		return *this;
	}

	dynamic_bitset& flip() noexcept
	{
		for (auto& block : bits_) {
			block = ~block;
		}
		zero_unused_bits();
		return *this;
	}

	dynamic_bitset& flip(size_type position)
	{
		assert(position < num_bits_);
		bits_[block_index(position)] ^= bit_mask(position);
		return *this;
	}

	dynamic_bitset operator~() const noexcept
	{
		dynamic_bitset other(*this);
		other.flip();
		return other;
	}
#pragma endregion

#pragma region Element access
	constexpr bool operator[](size_type position) const
	{
		return test(position);
	}

	reference operator[](size_type position)
	{
		return reference(bits_[block_index(position)], bit_index(position));
	}

	bool test(size_type position) const
	{
		assert(position < num_bits_);
		return (bits_[block_index(position)] & bit_mask(position)) != 0;
	}

	bool all() const noexcept
	{
		if (empty()) {
			return true;
		}
		const auto extra_bits = bit_index(size());
		auto const all_ones = static_cast<block_type>(~0);

		for (size_type i = 0, e = num_blocks() - 1; i < e; ++i) {
			if (bits_[i] != all_ones) {
				return false;
			}
		}
		if (extra_bits == 0) {
			return bits_.back() == all_ones;
		} else {
			const block_type mask = (block_type(1) << extra_bits) - 1;
			if (bits_.back() != mask) {
				return false;
			}
		}
		return true;
	}

	bool any() const noexcept
	{
		for (auto block : bits_) {
			if (block) {
				return true;
			}
		}
		return false;
	}

	bool none() const noexcept
	{
		return !any();
	}

	size_type count() const noexcept
	{
		size_type count = 0;
		for (auto block : bits_) {
			count += __builtin_popcount(block);
		}
		return count;
	}
#pragma endregion

#pragma region Iterators
	iterator begin() noexcept
	{
		return iterator(bits_.begin(), bit_index(0));
	}

	iterator end() noexcept
	{
		return iterator(bits_.end() - 1, bit_index(num_bits_));
	}
#pragma endregion

#pragma region Capacity
	constexpr size_type size() const noexcept
	{
		return num_bits_;
	}

	constexpr size_type num_blocks() const noexcept
	{
		return bits_.size();
	}

	constexpr auto empty() const noexcept
	{
		return num_bits_ == 0;
	}
#pragma endregion

	// TODO
	dynamic_bitset operator<<(size_type position) const noexcept;
	dynamic_bitset operator>>(size_type position) const noexcept;

private:
	constexpr size_type block_index(size_type position) const noexcept
	{
		return position / block_width;
	}

	constexpr block_width_type bit_index(size_type position) const noexcept
	{
		return static_cast<block_width_type>(position % block_width);
	}

	constexpr block_type bit_mask(size_type position) const noexcept
	{
		return (block_type(1) << bit_index(position));
	}

	constexpr size_type calculate_num_blocks(size_type num_bits) const
	{
		return num_bits / block_width + static_cast<size_type>(num_bits % block_width != 0);
	}

	template<typename ValueType>
	void init_from_value(size_type num_bits, ValueType value)
	{
		static_assert(std::is_integral<ValueType>::value, "Integral type required.");
		assert(bits_.size() == 0);
		static constexpr size_type value_width = std::numeric_limits<ValueType>::digits;
		bits_.resize(calculate_num_blocks(num_bits));
		num_bits_ = num_bits;

		if (num_bits < value_width) {
			const auto mask = (ValueType(1) << num_bits) - 1;
			value &= mask;
		}

		auto it = bits_.begin();
		if constexpr (block_width >= value_width) {
			*it = static_cast<block_type>(value);
		} else {
			for (; value; value >>= block_width, ++it) {
				*it = static_cast<block_type>(value);
			}
		}
	}

	void zero_unused_bits()
	{
		assert(num_blocks() == calculate_num_blocks(num_bits_));
		const auto extra_bits = bit_index(size());
		if (extra_bits != 0) {
			bits_.back() &= (block_type(1) << extra_bits) - 1;
		}
	}

private:
	size_type num_bits_;
	container_type bits_;
};

} // namespace tweedledum
