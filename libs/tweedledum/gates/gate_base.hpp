/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../utils/angle.hpp"
#include "gate_set.hpp"

#include <cstdint>
#include <fmt/format.h>
#include <ostream>

namespace tweedledum {

/*! \brief Simple class to hold information about the operation of a gate */
class gate_base {
public:
#pragma region Constructors
	constexpr gate_base(gate_set operation)
	    : operation_(operation)
	    , theta_(angles::zero)
	    , phi_(angles::zero)
	    , lambda_(angles::zero)
	{
		assert(!(is_single_qubit() && is_gate()));
	}

	constexpr gate_base(gate_set operation, angle theta, angle phi, angle lambda)
	    : operation_(operation)
	    , theta_(theta)
	    , phi_(phi)
	    , lambda_(lambda)
	{}

	constexpr gate_base(gate_set operation, angle rotation_angle)
	    : operation_(operation)
	    , theta_(angles::zero)
	    , phi_(angles::zero)
	    , lambda_(rotation_angle)
	{
		assert(is_one_of(gate_set::rotation_x, gate_set::rotation_z));
		if (operation == gate_set::rotation_x) {
			theta_ = rotation_angle;
			phi_ = -angles::one_half;
			lambda_ = angles::one_half;
		}
	}
#pragma endregion

#pragma region Operation properties
	/*! \brief Returns the adjoint operation. */
	constexpr gate_set adjoint() const
	{
		return detail::gates_info[static_cast<uint8_t>(operation_)].adjoint;
	}

	/*! \brief Returns true if this gate is the operation ``operation``. */
	constexpr bool is(gate_set op) const
	{
		return operation() == op;
	}

	template<typename... OPs>
	constexpr bool is_one_of(gate_set op) const
	{
		return is(op);
	}

	/*! \brief Returns true if this gate is one of the operations ``{op0, op1, .. , opN}``. */
	template<typename... OPs>
	constexpr bool is_one_of(gate_set op0, OPs... opN) const
	{
		return is(op0) || is_one_of(opN...);
	}

	/*! \brief Returns true if this is a meta gate. */
	constexpr bool is_meta() const
	{
		return (operation_ < gate_set::identity || operation_ == gate_set::num_defined_ops);
	}

	/*! \brief Returns true if this gate is a quantum unitary operation. */
	constexpr bool is_gate() const
	{
		return (!is_meta());
	}

	/*! \brief Returns true if this gate acts on one I/Os. */
	constexpr bool is_one_io() const
	{
		return (operation_ >= gate_set::input && operation_ <= gate_set::t_dagger);
	}

	/*! \brief Returns true if this gate acts on two I/Os. */
	constexpr bool is_two_io() const
	{
		return is_one_of(gate_set::cx, gate_set::cz, gate_set::swap, gate_set::measurement);
	}

	/*! \brief Returns true if this gate acts on a single qubit. */
	// TODO: Is MEASUREMENT single-qubit? It acts on two I/Os, but only one qubit.
	constexpr bool is_single_qubit() const
	{
		return (operation_ >= gate_set::identity && operation_ <= gate_set::t_dagger);
	}

	/*! \brief Returns true if this gate acts on two _qubits_. */
	constexpr bool is_double_qubit() const
	{
		return is_one_of(gate_set::cx, gate_set::cz, gate_set::swap);
	}

	/*! \brief Returns true if this gate is a rotation around x axis. */
	constexpr bool is_x_rotation() const
	{
		return detail::gates_info[static_cast<uint8_t>(operation_)].rotation_axis == 'x';
	}

	/*! \brief Returns true if this gate is a rotation around y axis. */
	constexpr bool is_y_rotation() const
	{
		return detail::gates_info[static_cast<uint8_t>(operation_)].rotation_axis == 'y';
	}

	/*! \brief Returns true if this gate is a rotation around z axis. */
	constexpr bool is_z_rotation() const
	{
		return detail::gates_info[static_cast<uint8_t>(operation_)].rotation_axis == 'z';
	}

	/*! \brief Returns the operation. (see ``gate_set``) */
	constexpr gate_set operation() const
	{
		return operation_;
	}

	/*! \brief Return gate symbol. (see ``gate_set``) */
	std::string symbol() const
	{
		return detail::gates_info[static_cast<uint8_t>(operation_)].symbol;
	}
#pragma endregion

#pragma region Angle information
	/* !brief Return the rotation angle */
	constexpr angle rotation_angle() const
	{
		// assert(is_single_qubit());
		if (is_z_rotation()) {
			return lambda_;
		}
		return theta_;
	}
#pragma endregion

#pragma region Overloads
	friend std::ostream& operator<<(std::ostream& out, gate_base const& operation)
	{
		out << detail::gates_info[static_cast<uint8_t>(operation.operation_)].name;
		return out;
	}
#pragma endregion

private:
	gate_set operation_;
	angle theta_;
	angle phi_;
	angle lambda_;
};

namespace gate {

/* Single-qubit gates */
constexpr gate_base identity(gate_set::identity, angles::zero, angles::zero, angles::zero);
constexpr gate_base hadamard(gate_set::hadamard, angles::one_half, angles::zero, angles::one);
constexpr gate_base pauli_x(gate_set::pauli_x, angles::one, angles::zero, angles::one);
constexpr gate_base t(gate_set::t, angles::zero, angles::zero, angles::one_quarter);
constexpr gate_base phase(gate_set::phase, angles::zero, angles::zero, angles::one_half);
constexpr gate_base pauli_z(gate_set::pauli_z, angles::zero, angles::zero, angles::one);
constexpr gate_base phase_dagger(gate_set::phase_dagger, angles::zero, angles::zero, -angles::one_half);
constexpr gate_base t_dagger(gate_set::t_dagger, angles::zero, angles::zero, -angles::one_quarter);

/* Double-qubit unitary gates */
constexpr gate_base cx(gate_set::cx, angles::one, angles::zero, angles::one);
constexpr gate_base cz(gate_set::cz, angles::zero, angles::zero, angles::one);
constexpr gate_base swap(gate_set::swap);

/* Multiple-qubit unitary gates */
constexpr gate_base mcx(gate_set::mcx, angles::one, angles::zero, angles::one);
constexpr gate_base mcz(gate_set::mcz, angles::zero, angles::zero, angles::one);

/* Single-qubit, single-cbit gate */
constexpr gate_base measurement(gate_set::measurement);

} // namespace gate

} // namespace tweedledum