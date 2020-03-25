/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../networks/io_id.hpp"
#include "gate_base.hpp"
#include "gate_lib.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace tweedledum {

/*! \brief Three I/Os gate (defaul gate)
 *
 * This class represents a gate which can act upon one, two, or three I/Os of a
 * quantum network. Identifier(s) 'id' indicate on which I/O(s) the gate is
 * acting. At least one I/O must be the target, but, in case of SWAPs and
 * MEASUREMENTS, the gate will have two targets.
 */
class io3_gate final : public gate_base {
  private:
    constexpr static uint32_t invalid_value = 3u;

  public:
#pragma region Constants
    constexpr static uint32_t max_num_io = 3u;
#pragma endregion

#pragma region Constructors
    io3_gate(gate_base const& op, io_id target)
        : gate_base(op), num_controls_(0), num_targets_(1),
          control0_(invalid_value), control1_(invalid_value), target0_(0),
          target1_(invalid_value), ids_({target, io_invalid, io_invalid}) {
        assert(is_one_io());
    }

    // When dealing with controlled gates (e.g.CX, CZ) id0 is the control and
    // id1 the target
    // *) In case of SWAP they are both targets
    // *) In case of MEASUREMENT they are both targets and id1 _must_ be the
    // cbit
    io3_gate(gate_base const& op, io_id id0, io_id id1)
        : gate_base(op), ids_({io_invalid, io_invalid, io_invalid}) {
        init_two_io(id0, id1);
    }

    io3_gate(gate_base const& op, std::vector<io_id> const& controls,
             std::vector<io_id> const& targets)
        : gate_base(op), ids_({io_invalid, io_invalid, io_invalid}) {
        assert(targets.size() >= 1u &&
               "The gate must have at least one target");
        assert(targets.size() <= 2u && "The gate must have at most two target");
        assert(controls.size() + targets.size() <= max_num_io);

        switch (controls.size()) {
            case 0u:
                if (targets.size() == 1) {
                    init_one_io(targets[0]);
                } else if (targets.size() == 2) {
                    assert(is_one_of(gate_lib::swap, gate_lib::measurement));
                    init_two_io(targets[0], targets[1]);
                }
                break;

            case 1u:
                assert(targets.size() == 1 &&
                       "This gate operation cannot have morew than one target");
                assert(is_one_of(gate_lib::cx, gate_lib::cz));
                init_two_io(controls[0], targets[0]);
                break;

            case 2u:
                assert(targets.size() == 1 &&
                       "This gate operation cannot have morew than one target");
                assert(is_one_of(gate_lib::mcx, gate_lib::mcz));
                num_controls_ = 2;
                num_targets_ = 1;
                target1_ = invalid_value;
                ids_ = {controls[0], controls[1], targets[0]};
                std::sort(ids_.begin(), ids_.end());
                assert(ids_[0] != ids_[1] && ids_[1] != ids_[2] &&
                       "The I/Os must be different");
                target0_ = std::distance(
                    ids_.begin(),
                    std::find(ids_.begin(), ids_.end(), targets[0]));
                switch (target0_) {
                    case 0u:
                        control0_ = 1;
                        control1_ = 2;
                        break;

                    case 1u:
                        control0_ = 0;
                        control1_ = 2;
                        break;

                    case 2u:
                        control0_ = 0;
                        control1_ = 1;
                        break;

                    default:
                        assert(0);
                        std::abort();
                }
                break;

            default:
                assert(0);
                std::abort();
        }
    }
#pragma endregion

#pragma region Properties
    uint32_t num_controls() const { return num_controls_; }

    uint32_t num_targets() const { return num_targets_; }

    io_id control() const {
        if (num_controls() != 1) {
            return io_invalid;
        }
        return ids_[control0_];
    }

    io_id target() const {
        if (num_targets() != 1) {
            return io_invalid;
        }
        return ids_[target0_];
    }

    uint32_t qubit_slot(io_id qid) const {
        for (auto i = 0u; i < ids_.size(); ++i) {
            if (ids_[i].index() == qid.index()) {
                return i;
            }
        }
        // This is unreachable
        assert(0);
        std::abort();
    }

    // TODO: double check
    bool is_adjoint(io3_gate const& other) const {
        if (this->adjoint() != other.operation()) {
            return false;
        }
        if (data_ != other.data_) {
            return false;
        }
        for (uint32_t i = 0; i < max_num_io; ++i) {
            if (ids_[i] != other.ids_[i]) {
                return false;
            }
        }
        if (this->is_one_of(gate_lib::rotation_x, gate_lib::rotation_y,
                            gate_lib::rotation_z)) {
            if (this->rotation_angle() + other.rotation_angle() !=
                angles::zero) {
                return false;
            }
        }
        return true;
    }
#pragma endregion

#pragma region Const iterators
    template <typename Fn>
    void foreach_control(Fn&& fn) const {
        if (control0_ != invalid_value) {
            fn(ids_[control0_]);
        }
        if (control1_ != invalid_value) {
            fn(ids_[control1_]);
        }
    }

    template <typename Fn>
    void foreach_target(Fn&& fn) const {
        fn(ids_[target0_]);
        if (target1_ != invalid_value) {
            fn(ids_[target1_]);
        }
    }
#pragma endregion

  private:
    void init_one_io(io_id target) {
        num_controls_ = 0;
        num_targets_ = 1;
        target0_ = 0;
        target1_ = invalid_value;
        control0_ = invalid_value;
        control1_ = invalid_value;
        ids_ = {target, io_invalid, io_invalid};
    }

    void init_two_io(io_id id0, io_id id1) {
        assert(is_two_io() && "The operation must act on two I/Os");
        assert(id0 != id1 && "The I/Os must be different");
        num_controls_ = 1;
        num_targets_ = 1;
        control0_ = 0;
        control1_ = invalid_value;
        target0_ = 1;
        target1_ = invalid_value;
        ids_ = {id0, id1, io_invalid};

        if (id0 < id1) {
            std::swap(ids_[0], ids_[1]);
            control0_ = 1;
            target0_ = 0;
        }
        if (is_one_of(gate_lib::swap, gate_lib::measurement)) {
            target1_ = control0_;
            control0_ = invalid_value;
            num_controls_ = 0;
            num_targets_ = 2;
            if (is(gate_lib::measurement)) {
                assert(!id1.is_qubit() &&
                       "In a measurement gate the second I/O must be a cbit");
            }
        }
    }

  private:
    union {
        uint32_t data_;
        struct {
            uint32_t unused_ : 20;
            /*! \brief indicates the slot which holds the qid of the target. */
            uint32_t num_controls_ : 2;
            uint32_t num_targets_ : 2;
            uint32_t control0_ : 2;
            uint32_t control1_ : 2;
            uint32_t target0_ : 2;
            uint32_t target1_ : 2;
        };
    };

    /*! \brief an array which hold the qids' of the qubits this gate is acting
     * upon */
    std::array<io_id, max_num_io> ids_;
};

} // namespace tweedledum
