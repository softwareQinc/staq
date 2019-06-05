/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../utils/angle.hpp"
#include "gate_lib.hpp"

#include <cstdint>
#include <fmt/format.h>
#include <ostream>

namespace tweedledum {

/*! \brief Simple class to hold information about the operation of a gate */
class gate_base {
public:
#pragma region Constructors
	constexpr gate_base(gate_lib operation)
	    : operation_(operation)
	    , theta_(angles::zero)
	    , phi_(angles::zero)
	    , lambda_(angles::zero)
	{
		assert(!(is_single_qubit() && is_gate()));
	}

	constexpr gate_base(gate_lib operation, angle theta, angle phi, angle lambda)
	    : operation_(operation)
	    , theta_(theta)
	    , phi_(phi)
	    , lambda_(lambda)
	{}

	constexpr gate_base(gate_lib operation, angle rotation_angle)
	    : operation_(operation)
	    , theta_(angles::zero)
	    , phi_(angles::zero)
	    , lambda_(rotation_angle)
	{
		assert(is_one_of(gate_lib::rotation_x, gate_lib::rotation_z));
		if (operation == gate_lib::rotation_x) {
			theta_ = rotation_angle;
			phi_ = -angles::pi_half;
			lambda_ = angles::pi_half;
		}
	}
#pragma endregion

#pragma region Operation properties
	/*! \brief Returns the adjoint operation. */
	constexpr gate_lib adjoint() const
	{
		return detail::gates_info[static_cast<uint8_t>(operation_)].adjoint;
	}

	/*! \brief Returns true if this gate is the operation ``operation``. */
	constexpr bool is(gate_lib op) const
	{
		return operation() == op;
	}

	template<typename... OPs>
	constexpr bool is_one_of(gate_lib op) const
	{
		return is(op);
	}

	/*! \brief Returns true if this gate is one of the operations ``{op0, op1, .. , opN}``. */
	template<typename... OPs>
	constexpr bool is_one_of(gate_lib op0, OPs... opN) const
	{
		return is(op0) || is_one_of(opN...);
	}

	/*! \brief Returns true if this is a meta gate. */
	constexpr bool is_meta() const
	{
		return (operation_ < gate_lib::identity || operation_ == gate_lib::num_defined_ops);
	}

	/*! \brief Returns true if this gate is a quantum unitary operation. */
	constexpr bool is_gate() const
	{
		return (!is_meta());
	}

	/*! \brief Returns true if this gate acts on one I/Os. */
	constexpr bool is_one_io() const
	{
		return (operation_ >= gate_lib::input && operation_ <= gate_lib::t_dagger);
	}

	/*! \brief Returns true if this gate acts on two I/Os. */
	constexpr bool is_two_io() const
	{
		return is_one_of(gate_lib::cx, gate_lib::cz, gate_lib::swap, gate_lib::measurement);
	}

	/*! \brief Returns true if this gate acts on a single qubit. */
	// TODO: Is MEASUREMENT single-qubit? It acts on two I/Os, but only one qubit.
	constexpr bool is_single_qubit() const
	{
		return (operation_ >= gate_lib::identity && operation_ <= gate_lib::t_dagger);
	}

	/*! \brief Returns true if this gate acts on two _qubits_. */
	constexpr bool is_double_qubit() const
	{
		return is_one_of(gate_lib::cx, gate_lib::cz, gate_lib::swap);
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

	/*! \brief Returns the operation. (see ``gate_lib``) */
	constexpr gate_lib operation() const
	{
		return operation_;
	}

	/*! \brief Return gate symbol. (see ``gate_lib``) */
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
	gate_lib operation_;
	angle theta_;
	angle phi_;
	angle lambda_;
};

namespace gate {

/* Single-qubit gates */
constexpr gate_base identity(gate_lib::identity, angles::zero, angles::zero, angles::zero);
constexpr gate_base hadamard(gate_lib::hadamard, angles::pi_half, angles::zero, angles::pi);
constexpr gate_base pauli_x(gate_lib::pauli_x, angles::pi, angles::zero, angles::pi);
constexpr gate_base pauli_y(gate_lib::pauli_y, angles::pi, angles::pi_half, angles::pi_half);
constexpr gate_base t(gate_lib::t, angles::zero, angles::zero, angles::pi_quarter);
constexpr gate_base phase(gate_lib::phase, angles::zero, angles::zero, angles::pi_half);
constexpr gate_base pauli_z(gate_lib::pauli_z, angles::zero, angles::zero, angles::pi);
constexpr gate_base phase_dagger(gate_lib::phase_dagger, angles::zero, angles::zero, -angles::pi_half);
constexpr gate_base t_dagger(gate_lib::t_dagger, angles::zero, angles::zero, -angles::pi_quarter);

/* Double-qubit unitary gates */
constexpr gate_base cx(gate_lib::cx, angles::pi, angles::zero, angles::pi);
constexpr gate_base cz(gate_lib::cz, angles::zero, angles::zero, angles::pi);
constexpr gate_base swap(gate_lib::swap);

/* Multiple-qubit unitary gates */
constexpr gate_base mcx(gate_lib::mcx, angles::pi, angles::zero, angles::pi);
constexpr gate_base mcz(gate_lib::mcz, angles::zero, angles::zero, angles::pi);

/* Single-qubit, single-cbit gate */
constexpr gate_base measurement(gate_lib::measurement);

} // namespace gate

} // namespace tweedledum