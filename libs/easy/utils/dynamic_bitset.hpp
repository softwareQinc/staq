/* easy: C++ ESOP library
 * Copyright (C) 2018  EPFL
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

/*!
  \file dynamic_bitset.hpp
  \brief A minimalistic excerpt from boost::dynamic_bitset.

  \author Heinz Riener
*/

#pragma once

#include <vector>
#include <cassert>

namespace easy::utils
{

template<typename Block = uint32_t, typename Allocator = std::allocator<Block>>
class dynamic_bitset;

/*! \brief dynamic_bitset
 *
 * An implementation of a bitset with non-predefined size.
 */
template<typename Block, typename Allocator>
class dynamic_bitset
{
public:
  using block_type = Block;
  using allocator_type = Allocator;
  using size_type = uint64_t;
  using block_width_type = block_type;

public:
  static constexpr block_width_type bits_per_block = std::numeric_limits<Block>::digits;
  static constexpr size_type npos = static_cast<size_type>( -1 );

public:
  /*! \brief Constructor of dynamic_bitset
   *
   * \param alloc An allocator of the blocks.
   */
  explicit dynamic_bitset( const Allocator& alloc = Allocator() )
    : _bits( alloc )
  {}

  /*! \brief Copy constructor of dynamic_bitset
   *
   * \param other Another dynamic_bitset.
   */
  explicit dynamic_bitset( const dynamic_bitset<>& other )
    : _bits( other._bits )
    , _num_bits( other._num_bits )
  {}

  /*! \brief Destructor of dynamic_bitset */
  ~dynamic_bitset() {}

  /*! \brief Requests that the capacity be at least enough to contain num_bits
   *
   * \param num_bits Number of bits to be reserved
   */
  void reserve( size_type num_bits )
  {
    _bits.reserve( calc_num_blocks( num_bits ) );
  }

  /*! \brief Resizes the container so that it contains num_bits
   *
   * If num_bits is smaller than the current container size, the
   * content is reduced to its first num_bits, removing those beyond.
   * If num_bits is greater than the current size, the content is
   * expanded by inserting at the end as many bits as needed to reach
   * the size of num_bits.  If value is specified, the new bits are
   * initialized as copies of value, otherwise, they are initialized
   * with false.  If num_bits is also greater than the current
   * container capacity, an automatic reallocation of the allocated
   * storage space takes place.
   *
   * \param num_bits Number of bits to be contained
   * \param value Initial value of bits added to the container
   */
  void resize( size_type num_bits, bool value = false )
  {
    const size_type old_num_blocks = num_blocks();
    const size_type required_blocks = calc_num_blocks( num_bits );

    const block_type v = value ? ~Block( 0 ) : Block( 0 );

    if ( required_blocks != old_num_blocks )
    {
      _bits.resize( required_blocks, v );
    }

    // At this point:
    //
    //  - if the buffer was shrunk, we have nothing more to do,
    //    except a call to _zero_unused_bits()
    //
    //  - if it was enlarged, all the (used) bits in the new blocks have
    //    the correct value, but we have not yet touched those bits, if
    //    any, that were 'unused bits' before enlarging: if value == true,
    //    they must be set.

    if ( value && ( num_bits > _num_bits ) )
    {
      const block_width_type extra_bits = count_extra_bits();
      if ( extra_bits )
      {
        assert( old_num_blocks >= 1 && old_num_blocks <= _bits.size() );

        // Set them.
        _bits[old_num_blocks - 1] |= ( v << extra_bits );
      }
    }

    _num_bits = num_bits;
    _zero_unused_bits();
  }

  /*! \brief Removes all bits from the dynamic_bitset, leaving the container with a size of 0. */
  void clear() // no throw
  {
    _bits.clear();
    _num_bits = 0;
  }

  /*! \brief Adds a new value at the end of dynamic_bitset, after its
   *  current last bit.  The content of value is copied to the end of the
   *  dynamic_bitset.
   *
   * \param value Value to be pushed to the dynamic_bitset
   */
  void push_back( bool value )
  {
    const size_type sz = num_bits();
    resize( sz + 1 );
    set_bit( sz, value );
  }

  /* \brief Append a block to the dynamic_bitset
   *
   * \param block Block to be appended
   */
  void append( Block value )
  {
    const block_width_type r = count_extra_bits();

    if ( r == 0 )
    {
      // the buffer is empty, or all blocks are filled
      _bits.push_back( value );
    }
    else
    {
      _bits.push_back( value >> ( bits_per_block - r ) );
      _bits[_bits.size() - 2] |= ( value << r ); // _bits.size() >= 2
    }

    _num_bits += bits_per_block;
  }

  /*! \brief Size of the dynamic_bitset
   *
   * Returns the number of allocated bits in the blocks storage.
   */
  size_type num_bits() const noexcept
  {
    return _num_bits;
  }

  /*! \brief Number of blocks currently used by the dynamic_bitset
   *
   * Returns the number of blocks allocated for storing the bits.
   */
  size_type num_blocks() const noexcept
  {
    return _bits.size();
  }

  /*! \brief Checks if no bit is stored in dynamic_bitset
   *
   * Returns true if no bit is stored in the block storage.
   */
  bool empty() const noexcept
  {
    return num_bits() == 0;
  }

  /*! \brief Capacity of the dynamic_bitset
   *
   * Returns the number of bits that the container has currently allocated space for.
   */
  size_type capacity() const noexcept
  {
    return _bits.capacity() * bits_per_block;
  }

  /*! \brief Sets a bit at a position
   *
   * \param pos A position in the dynamic_bitset
   * \param val Value to be set
   *
   * Returns a reference to the current dynamic_bitset
   */
  dynamic_bitset& set_bit( size_type pos, bool value = true )
  {
    assert( pos < _num_bits && "Index access out-of-bounds" );

    if ( value )
    {
      _bits[block_index( pos )] |= bit_mask( pos );
    }
    else
    {
      reset_bit( pos );
    }

    return *this;
  }

  /*! \brief Resets a bit at a position
   *
   * \param pos A position in the dynamic_bitset
   *
   * Returns a reference to the current dynamic_bitset
   */
  dynamic_bitset& reset_bit( size_type pos )
  {
    assert( pos < _num_bits  && "Index access out-of-bounds" );
    _bits[block_index( pos )] &= ~bit_mask( pos );
    return *this;
  }

  /*! \brief Resets all bits in the dynamic_bitset
   *
   * Returns a reference to the current dynamic_bitset
   */
  dynamic_bitset& reset()
  {
    std::fill( _bits.begin(), _bits.end(), Block( 0 ) );
    return *this;
  }

  /*! \brief Flips a bit at a position
   *
   * Returns a reference to the current dynamic_bitset
   */
  dynamic_bitset& flip_bit( size_type pos )
  {
    assert( pos < _num_bits  && "Index access out-of-bounds" );
    _bits[block_index( pos )] ^= bit_mask( pos );
    return *this;
  }

  /*! \brief Gets the bit at a position
   *
   * Returns the current value of a bit at a position
   */
  bool operator[]( size_type pos ) const
  {
    return test( pos );
  }

  /*! \brief Gets the bit at a position
   *
   * Returns the current value of a bit at a position
   */
  bool test( size_type pos ) const
  {
    assert( pos < _num_bits  && "Index access out-of-bounds" );
    return ( _bits[block_index( pos )] & bit_mask( pos ) ) != 0;
  }

public:
  /*! \brief Compute the block of a position
   *
   * \param pos Position in a dynamic_bitset
   * Returns the index of the block of a position
   */
  static size_type block_index( size_type pos )
  {
    return pos / bits_per_block;
  }

  /*! \brief Compute the bit within a block of a position
   *
   * \param pos Position in a dynamic_bitset
   * Returns the index within a block of a position
   */
  static block_width_type bit_index( size_type pos )
  {
    return static_cast<block_width_type>( pos % bits_per_block );
  }

protected:
  static Block bit_mask( size_type pos )
  {
    return Block( 1 ) << bit_index( pos );
  }

  static size_type calc_num_blocks( size_type num_bits )
  {
    return num_bits / bits_per_block + static_cast<size_type>( num_bits % bits_per_block != 0 );
  }

  block_width_type count_extra_bits() const
  {
    return bit_index( num_bits() );
  }

  Block& _highest_block()
  {
    return const_cast<Block &>( static_cast<const dynamic_bitset *>( this )->_highest_block() );
  }

  const Block& _highest_block() const
  {
    assert( num_bits() > 0 && num_blocks() > 0 );
    return _bits.back();
  }

  /*! \brief Resets unused bits
   *
   * If num_bits() is not a multiple of bits_per_block then not all the
   * bits in the last block are used.  This function resets the unused
   * bits (convenient for the implementation of many member functions)
   */
  void _zero_unused_bits()
  {
    assert ( num_blocks() == calc_num_blocks( _num_bits ) );

    // if != 0 this is the number of bits used in the last block
    const block_width_type extra_bits = count_extra_bits();

    if (extra_bits != 0)
    {
      _highest_block() &= ~( ~static_cast<Block>( 0 ) << extra_bits );
    }
  }

protected:
  std::vector<Block> _bits; /*!< Vector of bits encoding the bits */
  size_type _num_bits{0}; /*!< Number of allocated bits */
}; /* dynamic_bitset */

} // namespace easy::utils
