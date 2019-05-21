/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

static_assert(false,
              "file netowks/interface.hpp cannot be included, it's only used for documentation");

namespace tweedledum {

template<typename GateType>
class network final {
public:
#pragma region Types and constructors
	/*! \brief Type referring to itself.
	 *
	 * The ``base_type`` is the network type itself. It is required, because views may extend
	 * networks, and this type provides a way to determine the underlying network type.
	 */
	using base_type = network;

	/*! \brief Type representing a gate.
	 *
	 * A ``Gate`` is an operation that can be applied to a collection of qubits. It could be a
	 * meta operation, such as, primary input and a primary output, or a unitary operation gate.
	 */
	using gate_type = GateType;

	/*! \brief Type representing a node.
	 *
	 * A ``node`` is a node in the network. Each node must contains a gate.
	 */
	struct node_type {};

	/*! \brief Type representing the storage.
	 *
	 * A ``storage`` is some container that can contain all data necessary to store the network.
	 * It can constructed outside of the network and passed as a reference to the constructor.
	 * It may be shared among several networks. A `std::shared_ptr<T>` is a convenient data
	 * structure to hold a storage.
	 */
	struct storage_type {};

	network();

	explicit network(storage s);
#pragma endregion

#pragma region I / O and ancillae qubits
	/*! \brief Creates a labeled qubit in the network and returns its ``io_id``
	 */
	io_id add_qubit(std::string const& qlabel);

	/*! \brief Creates a unlabeled qubit in the network and returns its ``io_id``
	 *
	 * Since all I/Os in a network must be labeled, this function will create
	 * a generic label with the form: qN, where N is the ``io_id``.
	 */
	io_id add_qubit();

	/*! \brief Creates a labeled cbit (classical bit) in the network and returns its ``io_id``
	 */
	io_id add_cbit(std::string const& label);

	/*! \brief Creates a unlabeled cbit (classical bit) in the network and returns its ``io_id``
	 *
	 * Since all I/Os in a network must be labeled, this function will create
	 * a generic label with the form: cN, where N is the ``io_id``.
	 */
	io_id add_cbit();
#pragma endregion

#pragma region Structural properties
	/*! \brief Returns the number of nodes. */
	uint32_t size() const;

	/*! \brief Returns the number of qubits. */
	uint32_t num_qubits() const;

	/*! \brief Returns the number of classical bits. */
	uint32_t num_cbits() const;

	/*! \brief Returns the number of gates, i.e., nodes that hold unitary operations */
	uint32_t num_gates() const;
#pragma endregion

#pragma region Nodes
	/*! \brief Get the node a node_ptr is pointing to. */
	auto& get_node(node_ptr node_ptr) const;

	/*! \brief Returns the index of a node.
	 *
	 * The index of a node must be a unique for each node and must be between 0 (inclusive) and
	 * the size of a network (exclusive, value returned by ``size()``).
	 */
	auto node_to_index(node const& node) const;
#pragma endregion

#pragma region Add gates(qids)
	/*! \brief Add a gate to the network. */
	node_type& add_gate(gate_type const& gate);

	/*! \brief Add an one-qubit gate to the network using io_id. */
	node_type& add_gate(gate_base op, io_id target);

	/*! \brief Add a controlled single-target gate to the network using qubit_ids. */
	node_type& add_gate(gate_base op, io_id control, io_id target);

	/*! \brief Add a multiple-controlled multiple-target gate to the network using qubit_ids */
	node_type& add_gate(gate_base op, std::vector<io_id> controls, std::vector<io_id> targets);
#pragma endregion

#pragma region Add gates(qlabels)
	/*! \brief Add an one-qubit gate to the network using qubit label (`qlabel`). */
	node_type& add_gate(gate_base op, std::string const& qlabel_target);

	/*! \brief Add a controlled single-target gate to the network using qubit labels (`qlabel`). */
	node_type& add_gate(gate_base op, std::string const& qlabel_control,
	                    std::string const& qlabel_target);

	/*! \brief Add a multiple-controlled multiple-target gate to the network qubit labels (`qlabel`). */
	node_type& add_gate(gate_base op, std::vector<std::string> const& qlabels_control,
	                    std::vector<std::string> const& qlabels_target);
#pragma endregion

#pragma region Rewiring
	void rewire(std::vector<uint32_t> const& rewiring_map)
	{
		storage_->rewiring_map = rewiring_map;
	}

	void rewire(std::vector<std::pair<uint32_t, uint32_t>> const& transpositions)
	{
		for (auto&& [i, j] : transpositions) {
			std::swap(storage_->rewiring_map[i], storage_->rewiring_map[j]);
		}
	}

	constexpr auto rewire_map() const
	{
		return storage_->rewiring_map;
	}
#pragma endregion

#pragma region Const iterators
	/*! \brief Calls ``fn`` on every qubit in the network.
	 *
	 * The paramater ``fn`` is any callable that must have one of the following three signatures.
	 * - ``void(uint32_t qid)``
	 * - ``void(string const& qlabel)``
	 * - ``void(uint32_t qid, string const& qlabel)``
	 */
	template<typename Fn>
	void foreach_qubit(Fn&& fn) const;

	/*! \brief Calls ``fn`` on every input node in the network.
	 *
	 * The paramater ``fn`` is any callable that must have one of the following two signatures.
	 * - ``void(node_type const& node)``
	 * - ``void(node_type const& node, uint32_t node_index)``
	 */
	template<typename Fn>
	void foreach_input(Fn&& fn) const;

	/*! \brief Calls ``fn`` on every output node in the network.
	 *
	 * The paramater ``fn`` is any callable that must have one of the following two signatures.
	 * - ``void(node_type const& node)``
	 * - ``void(node_type const& node, uint32_t node_index)``
	 */
	template<typename Fn>
	void foreach_output(Fn&& fn) cons;

	/*! \brief Calls ``fn`` on every unitrary gate node in the network.
	 *
	 * The paramater ``fn`` is any callable that must have one of the following four signatures.
	 * - ``void(node_type const& node)``
	 * - ``void(node_type const& node, uint32_t node_index)``
	 * - ``bool(node_type const& node)``
	 * - ``bool(node_type const& node, uint32_t node_index)``
	 *
	 * The paramater ``start`` is a index to a starting point.
	 *
	 * If ``fn`` returns a ``bool``, then it can interrupt the iteration by returning ``false``.
	 */
	template<typename Fn>
	void foreach_gate(Fn&& fn, uint32_t start = 0u) const;

	/*! \brief Calls ``fn`` on every node in the network.
	 *
	 * The paramater ``fn`` is any callable that must have one of the following four signatures.
	 * - ``void(node_type const& node)``
	 * - ``void(node_type const& node, uint32_t node_index)``
	 * - ``bool(node_type const& node)``
	 * - ``bool(node_type const& node, uint32_t node_index)``
	 *
	 * If ``fn`` returns a ``bool``, then it can interrupt the iteration by returning ``false``.
	 */
	template<typename Fn>
	void foreach_node(Fn&& fn) const;
#pragma endregion

#pragma region Const node iterators
	/*! \brief Calls ``fn`` on all children of a node.
	 *
	 * The paramater ``fn`` is any callable that must have one of the following three signatures.
	 * - ``void(node_ptr const&)``
	 * - ``void(node_ptr const&, uint32_t)``
	 *
	 * If ``fn`` has two parameters, the second parameter is an index starting
	 * from 0 and incremented in every iteration.
	 */
	template<typename Fn>
	void foreach_child(node_type const& node, Fn&& fn) const;
#pragma endregion

#pragma region Visited flags
	/*! \brief Reset all visited values to 0. */
	void clear_visited();

	/*! \brief Returns the visited value of a node. */
	auto visited(node_type const& node) const;

	/*! \brief Sets the visited value of a node. */
	void set_visited(node_type const& node, uint32_t value);
#pragma endregion

#pragma region Custom node values
	/*! \brief Reset all values to 0. */
	void clear_values() const;

	/*! \brief Returns value of a node. */
	uint32_t value(node_type const& node) const;

	/*! \brief Sets value of a node. */
	void set_value(node_type const& node, uint32_t value) const;

	/*! \brief Increments value of a node and returns *previous* value. */
	uint32_t incr_value(node_type const& node) const;

	/*! \brief Decrements value of a node and returns *new* value. */
	uint32_t decr_value(node_type const& node) const;
#pragma endregion
};

} // namespace tweedledum