/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt, Mathias Soeken
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../../gates/gate_base.hpp"
#include "../../../gates/gate_lib.hpp"
#include "../../../networks/io_id.hpp"

namespace tweedledum::detail {

/*! \brief 
 *                                                           ┌───┐
 *  ──────────────●───────────────────●─────────●─────────●──┤ T ├
 *                │                   │         │         │  └───┘
 *                │                   │       ┌─┴─┐┌───┐┌─┴─┐┌───┐
 *  ────●─────────┼─────────●─────────┼───────┤ X ├┤ ┴ ├┤ X ├┤ T ├
 *      │         │         │         │       └───┘└───┘└───┘└───┘
 *    ┌─┴─┐┌───┐┌─┴─┐┌───┐┌─┴─┐┌───┐┌─┴─┐                    ┌───┐
 *  ──┤ X ├┤ ┴ ├┤ X ├┤ T ├┤ X ├┤ ┴ ├┤ X ├────────────────────┤ T ├
 *    └───┘└───┘└───┘└───┘└───┘└───┘└───┘                    └───┘
 */
template<typename Network>
void ccz_(Network& network, io_id x, io_id y, io_id z)
{
	assert(!y.is_complemented() || x.is_complemented());
	assert(!z.is_complemented());
	network.add_gate(gate::cx, y.id(), z);
	network.add_gate(x.is_complemented() ? gate::t : gate::t_dagger, z);
	network.add_gate(gate::cx, x.id(), z);
	network.add_gate(gate::t, z);
	network.add_gate(gate::cx, y.id(), z);
	network.add_gate(y.is_complemented() ? gate::t : gate::t_dagger, z);
	network.add_gate(gate::cx, x.id(), z);
	network.add_gate(x.is_complemented() && !y.is_complemented()? gate::t_dagger : gate::t, z);
	
	network.add_gate(gate::cx, x.id(), y);
	network.add_gate(gate::t_dagger, y);
	network.add_gate(gate::cx, x.id(), y);
	network.add_gate(y.is_complemented() ? gate::t_dagger : gate::t, x);
	network.add_gate(x.is_complemented() ? gate::t_dagger : gate::t, y);
}

/*! \brief Important normalization: if only one negated, it must be x
 * because of normalization: y complemented => x complemented
 * better T gate parallelization at the expense of an extra CNOT gate
 * 
 *   ┌───┐          ┌───┐┌───┐┌───┐┌───┐     ┌───┐     
 * ──┤ T ├──●───────┤ X ├┤ ┴ ├┤ X ├┤ ┴ ├─────┤ X ├──●──
 *   └───┘  │       └─┬─┘└───┘└─┬─┘└───┘     └─┬─┘  │  
 *   ┌───┐┌─┴─┐       │  ┌───┐  │              │  ┌─┴─┐
 * ──┤ T ├┤ X ├──●────┼──┤ ┴ ├──●─────────●────┼──┤ X ├
 *   └───┘└───┘  │    │  └───┘            │    │  └───┘
 *   ┌───┐     ┌─┴─┐  │  ┌───┐          ┌─┴─┐  │       
 * ──┤ T ├─────┤ X ├──●──┤ T ├──────────┤ X ├──●───────
 *   └───┘     └───┘     └───┘          └───┘          
 */
template<typename Network>
void ccz_tpar(Network& network, io_id x, io_id y, io_id z)
{
	assert(!y.is_complemented() || x.is_complemented());
	assert(!z.is_complemented());
	network.add_gate(y.is_complemented() ? gate::t_dagger : gate::t, x.id());
	network.add_gate(x.is_complemented() ? gate::t_dagger : gate::t, y.id());
	network.add_gate(x.is_complemented() && !y.is_complemented() ? gate::t_dagger : gate::t, z);

	network.add_gate(gate::cx, x.id(), y.id());
	network.add_gate(gate::cx, y.id(), z);
	network.add_gate(gate::cx, z, x.id());

	network.add_gate(x.is_complemented() ? gate::t : gate::t_dagger, x);
	network.add_gate(gate::t_dagger, y.id());
	network.add_gate(gate::t, z);

	network.add_gate(gate::cx, y.id(), x.id());
	network.add_gate(y.is_complemented() ? gate::t : gate::t_dagger, x);
	network.add_gate(gate::cx, y.id(), z);
	network.add_gate(gate::cx, z, x.id());
	network.add_gate(gate::cx, x.id(), y.id());
}

/*! \brief 
 */
template<typename Network>
void ccz(Network& network, io_id x, io_id y, io_id z, bool use_t_par)
{
	if (use_t_par) {
		ccz_tpar(network, x, y, z);
	} else {
		ccz_(network, x, y, z);
	}
}

} // namespace tweedledum::detail