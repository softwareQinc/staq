/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

static_assert(false, "file gate/interface.hpp cannot be included, it's only "
                     "used for documentation");

namespace tweedledum {

class gate {
  public:
#pragma region Constants
    constexpr static uint32_t max_num_io;
    constexpr static uint32_t network_max_num_qubits;
#pragma endregion

#pragma region Constructors
    /*! \brief Construct a single qubit gate
     *
     * \param op the operation (must be a single qubit operation).
     * \param target qubit identifier of the target.
     */
    gate(gate_base const& op, io_id target);

    /*! \brief Construct a controlled gate
     *
     * \param controlled_op the operation (must be a two qubit controlled
     * operation). \param control qubit identifier of the control. \param target
     * qubit identifier of the target.
     */
    gate(gate_base const& controlled_op, io_id control, io_id target);

    /*! \brief Construct a gate using vectors
     *
     * \param unitary_op the operation (must be unitary operation).
     * \param control qubit(s) identifier of the control(s).
     * \param targets qubit identifier of the target.
     */
    gate(gate_base const& unitary_op, std::vector<io_id> const& controls,
         std::vector<io_id> const& targets);
#pragma endregion

#pragma region Properties
    /*! \brief Return the number of controls */
    uint32_t num_controls() const;

    /*! \brief Returns the number of targets. */
    uint32_t num_targets() const;

    /*! \brief Returns the qubit id of the target qubit.
     *
     * When there is multiple targets this function
     * must return `invalid_qid`.
     */
    io_id target() const;

    /*! \brief Returns the qubit id of the control qubit.
     *
     * When there is no controls or multiple controls this function
     * must return `invalid_qid`.
     */
    io_id control() const;

    /*! \brief Checks weather a qubit is a control for the gate */
    bool is_control(io_id qid) const;

    /*! \brief Returns the slot in which a io_id is stored in the gate
     *
     * The slot of a qubit is unique within the gate and ``io_id`` is
     * a unique qubit identifier within the circuit.
     */
    uint32_t qubit_slot(io_id qid) const;

    /*! \brief Check whether the this gate is adjoint of ``other`` gate.
     *
     * The conecept of _gate_ adjointness requires that gate operations to be
     * adjoint, and that both gates act on the same qubits in the same way,
     * i.e., same controls and/or same targets.
     */
    bool is_adjoint(gate const& other) const;
#pragma endregion

#pragma region Iterators
    /*! \brief Calls ``fn`` on every target qubit of the gate.
     *
     * The paramater ``fn`` is any callable that must have one of the following
     * two signatures.
     * - ``void(io_id)``
     * - ``bool(io_id)``
     *
     * If ``fn`` returns a ``bool``, then it can interrupt the iteration by
     * returning ``false``.
     */
    template <typename Fn>
    void foreach_control(Fn&& fn) const;

    /*! \brief Calls ``fn`` on every target qubit of the gate.
     *
     * The paramater ``fn`` is any callable that must have one of the following
     * signature.
     * - ``void(io_id)``
     */
    template <typename Fn>
    void foreach_target(Fn&& fn) const;
#pragma endregion
};

} // namespace tweedledum