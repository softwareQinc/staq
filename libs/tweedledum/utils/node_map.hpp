/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

/* Taken from mockturtle (: */

#include "../traits.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace tweedledum {

/*! \brief Associative container for network nodes
 *
 * This container helps to store values associated to nodes in a network.  The
 * container is initialized with a network to derive the size according to the
 * number of nodes. The container can be accessed via nodes, or indirectly
 * via node_ptr, from which the corresponding node is derived.
 *
 * The implementation uses a vector as underlying data_ structure which is
 * indexed by the node's index.
 *
 * **Required network functions:**
 * - `size`
 * - `get_node`
 * - `node_to_index`
 */
template<class T, class Network>
class node_map {
public:
	using node_type = typename Network::node_type;
	using node_ptr_type = typename Network::node_ptr_type;
	using reference = typename std::vector<T>::reference;
	using const_reference = typename std::vector<T>::const_reference;

public:
	/*! \brief Default constructor. */
	explicit node_map(Network const& network)
	    : network_(network)
	    , data_(std::make_shared<std::vector<T>>(network.size()))
	{}

	/*! \brief Constructor with default value.
	 *
	 * Initializes all values in the container to `init_value`.
	 */
	node_map(Network const& network, T const& init_value)
	    : network_(network)
	    , data_(std::make_shared<std::vector<T>>(network.size(), init_value))
	{}

	/*! \brief Mutable access to value by node. */
	reference operator[](node_type const& node)
	{
		assert(network_.node_to_index(node) < data_->size() && "index out of bounds");
		return (*data_)[network_.node_to_index(node)];
	}

	/*! \brief Constant access to value by node. */
	const_reference operator[](node_type const& node) const
	{
		assert(network_.node_to_index(node) < data_->size() && "index out of bounds");
		return (*data_)[network_.node_to_index(node)];
	}

	/*! \brief Mutable access to value by node_ptr.
	 *
	 * This method derives the node from the node_ptr.  If the node and node_ptr type
	 * are the same in the network implementation, this method is disabled.
	 */
	template<typename _Ntk = Network,
	         typename = std::enable_if_t<!std::is_same_v<typename _Ntk::node_ptr_type,
		                                             typename _Ntk::node_type>>>
	reference operator[](node_ptr_type const& f)
	{
		assert(network_.node_to_index(network_.get_node(f)) < data_->size()
		       && "index out of bounds");
		return (*data_)[network_.node_to_index(network_.get_node(f))];
	}

	/*! \brief Constant access to value by node_ptr.
	 *
	 * This method derives the node from the node_ptr.  If the node and node_ptr type
	 * are the same in the network implementation, this method is disabled.
	 */
	template<typename _Ntk = Network,
	         typename = std::enable_if_t<!std::is_same_v<typename _Ntk::node_ptr_type,
		                                             typename _Ntk::node_type>>>
	const_reference operator[](node_ptr_type const& f) const
	{
		assert(network_.node_to_index(network_.get_node(f)) < data_->size()
		       && "index out of bounds");
		return (*data_)[network_.node_to_index(network_.get_node(f))];
	}

	/*! \brief Resets the size of the map.
	 *
	 * This function should be called, if the network changed in size. Then,
	 * the map is cleared, and resized to the current network's size.  All
	 * values are initialized with `init_value`.
	 *
	 * \param init_value Initialization value after resize
	 */
	void reset(T const& init_value = {})
	{
		data_->clear();
		data_->resize(network_.size(), init_value);
	}

	/*! \brief Resizes the map.
	 *
	 * This function should be called, if the node_map's size needs to
	 * be changed without clearing its data.
	 *
	 * \param init_value Initialization value after resize
	 */
	void resize(T const& init_value = {})
	{
		if (network_.size() > data_->size()) {
			data_->resize(network_.size(), init_value);
		}
	}

private:
	Network const& network_;
	std::shared_ptr<std::vector<T>> data_;
};

} // namespace tweedledum