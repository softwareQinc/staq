/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <cstdint>
#include <limits>

namespace tweedledum {

/* \brief Simple class to hold a i/o indentifier ``id``
 *
 *  In tweedledum an I/O can be either a classical bit (cbit) or a quantum bit (qubit).
 */
class io_id {
public:
#pragma region Types and constructors
	constexpr io_id(uint32_t index, bool is_qubit)
	    : data_((index << 2) | (is_qubit ? 1 : 0))
	{}

	constexpr io_id(uint32_t index, bool is_qubit, bool is_complemented)
	    : data_((index << 2) | (is_complemented ? 2 : 0) | (is_qubit ? 1 : 0))
	{}
#pragma endregion

#pragma region Properties
	uint32_t index() const
	{
		return (data_ >> 2);
	}

	uint32_t literal() const
	{
		return (data_ >> 1);
	}

	/*! \brief Guarantee the return of an uncomplemented id */ 
	io_id id() const
	{
		return io_id(index(), is_qubit());
	}

	bool is_complemented() const
	{
		return (data_ & 2) == 2;
	}

	bool is_qubit() const
	{
		return (data_ & 1) == 1;
	}
#pragma endregion
	
#pragma region Modifiers
	void complement()
	{
		data_ ^= 2;
	}
#pragma endregion

#pragma region Overloads
	operator uint32_t() const
	{
		return (data_ >> 2);
	}

	io_id operator!() const
	{
		io_id complemented(*this);
		complemented.data_ ^= 2;
		return complemented;
	}

	bool operator<(io_id other) const
	{
		return data_ < other.data_;
	}

	bool operator==(io_id other) const
	{
		return data_ == other.data_;
	}

	bool operator!=(io_id other) const
	{
		return data_ != other.data_;
	}
#pragma endregion

private:
	uint32_t data_;
};

constexpr auto io_invalid = io_id(std::numeric_limits<uint32_t>::max(), true, true);

} // namespace tweedledum
