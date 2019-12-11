/*
 * This file is part of staq.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

/**
 * \file gates/channel.hpp
 * \brief Gates in the channel representation
 * 
 * Implements various utilities for working in the channel representation of 
 * Clifford + rotation gates. Specifically, representing Clifford gates
 * and circuits as a permutation of Paulis, and likewise representing 
 * rotation gates as sums of Paulis. See 
 * [An algorithm for the T-count](https://arxiv.org/abs/1308.4134) 
 * for an overview of the representation.
 */

#include "utils/angle.hpp"

#include <unordered_map>
#include <map>
#include <vector>
#include <variant>
#include <iostream>

namespace staq {
namespace gates {

  /**
   * \struct staq::gates::ChannelRepr
   * \brief Gates in the channel representation parameterized by an argument type
   *
   * Decouples the gates and gate logic from the representation of their arguments.
   *
   * \tparam qarg The type of arguments to gates. Must have an overload for both < and std::hash
   */
  template<typename qarg>
  struct ChannelRepr {

    /**
     * \class staq::gates::ChannelRepr::PauliOp
     * \brief The single qubit Pauli group
     */
    enum class PauliOp  : unsigned short { i = 0, x = 1, z = 2, y = 3 };

    /**
     * \class staq::gates::ChannelRepr::IPhase
     * \brief Pauli global phases (powers of i)
     */
    enum class IPhase : unsigned short { zero = 0, one = 1, two = 2, three = 3 };


    /**
     * \brief Multiplication overload for Pauli ops
     */
    friend inline PauliOp operator*(const PauliOp& p, const PauliOp& q) {
      return static_cast<PauliOp>(static_cast<unsigned short>(p) ^ static_cast<unsigned short>(q));
    }

    /**
     * \brief Assignment addition overload for Pauli ops
     */
    friend inline PauliOp& operator*=(PauliOp& p, const PauliOp& q) {
      p = p * q;
      return p;
    }

    /**
     * \brief Extraction operator overload for Pauli ops
     */
    friend std::ostream& operator<<(std::ostream& os, const PauliOp& p) {
      switch(p) {
      case PauliOp::i: os << "I"; break;
      case PauliOp::x: os << "X"; break;
      case PauliOp::z: os << "Z"; break;
      case PauliOp::y: os << "Y"; break;
      }

      return os;
    }

    /**
     * \brief Multiplication overload for Pauli phases
     */
    friend inline IPhase operator*(const IPhase& a, const IPhase& b) {
      return static_cast<IPhase>((static_cast<unsigned short>(a) + static_cast<unsigned short>(b)) % 4);
    }

    /**
     * \brief Assignment addition overload for Pauli phases
     */
    friend inline IPhase operator*=(IPhase& a, const IPhase& b) {
      a = a * b;
      return a;
    }

    /**
     * \brief Extraction operator overload for Pauli phases
     */
    friend std::ostream& operator<<(std::ostream& os, const IPhase& p) {
      switch(p) {
      case IPhase::zero: os << ""; break;
      case IPhase::one: os << "i"; break;
      case IPhase::two: os << "-"; break;
      case IPhase::three: os << "-i"; break;
      }

      return os;
    }

    /**
     * \brief Get the phase of a product of Paulis 
     * \note Uses a local static lookup table
     * \param p Pauli operator
     * \param q Pauli operator
     * \return The phase of p*q
     */
    inline static IPhase normal_phase(const PauliOp& p, const PauliOp& q) {
      static IPhase phase_mult_table[16] = { IPhase::zero, // II
                                             IPhase::zero, // XI
                                             IPhase::zero, // ZI
                                             IPhase::zero, // YI
                                             IPhase::zero, // IX
                                             IPhase::zero, // XX
                                             IPhase::one, // ZX
                                             IPhase::three, // YX
                                             IPhase::zero, // IZ
                                             IPhase::three, // XZ
                                             IPhase::zero, // ZZ
                                             IPhase::one, // YZ
                                             IPhase::zero, // IY
                                             IPhase::one, // XY
                                             IPhase::three, // ZY
                                             IPhase::zero /* YY */ };
    
      auto idx = (static_cast<unsigned short>(p) | (static_cast<unsigned short>(q) << 2));
      return phase_mult_table[idx % 16];
    }

    /**
     * \brief Check whether two Paulis commute
     * \note Uses a local static lookup table
     * \param p Pauli operator
     * \param q Pauli operator
     * \return True if p*q = q*p
     */
    inline static bool paulis_commute (const PauliOp& p, const PauliOp& q) {
      static bool commute_table[16] = { true, // II
                                        true, // XI
                                        true, // ZI
                                        true, // YI
                                        true, // IX
                                        true, // XX
                                        false, // ZX
                                        false, // YX
                                        true, // IZ
                                        false, // XZ
                                        true, // ZZ
                                        false, // YZ
                                        true, // IY
                                        false, // XY
                                        false, // ZY
                                        true /* YY */ };
    
      auto idx = (static_cast<unsigned short>(p) | (static_cast<unsigned short>(q) << 2));
      return commute_table[idx % 16];
    }

    /** 
     * \class staq::gates::ChannelRepr::Pauli
     * \brief Class representing an multi-qubit pauli operator 
     */
    class Pauli {
    public:
      /** @name Constructors */
      /**@{*/
      /** \brief Empty constructor */
      Pauli() {}
      /** \brief Constructs a Pauli from a qubit, PauliOp pair */
      Pauli(std::pair<qarg, PauliOp> gate) { pauli_[gate.first] = gate.second; }
      /** \brief Constructs a multi-qubit Pauli given a hash map from qubits to PauliOps */
      Pauli(const std::unordered_map<qarg, PauliOp>& pauli) : pauli_(pauli) {}
      /**@}*/

      /** @name Smart constructors */
      /**@{*/
      /** \brief Construct a single-qubit identity Pauli */
      static Pauli i(const qarg& q) { return Pauli(std::make_pair(q, PauliOp::i)); }
      /** \brief Construct a single-qubit X Pauli */
      static Pauli x(const qarg& q) { return Pauli(std::make_pair(q, PauliOp::x)); }
      /** \brief Construct a single-qubit Z Pauli */
      static Pauli z(const qarg& q) { return Pauli(std::make_pair(q, PauliOp::z)); }
      /** \brief Construct a single-qubit Y Pauli */
      static Pauli y(const qarg& q) { return Pauli(std::make_pair(q, PauliOp::y)); }
      /**@}*/

      /** @name Accessors */
      /**@{*/
      /** \brief Return the phase of the Pauli */
      IPhase phase() const { return phase_; }

      /** 
       * \brief Apply a function to each non-trivial Pauli gate
       *
       * \tparam Fn The type of the function to be applied. 
       *            Must be invocable on a (qarg, PauliOp) pair
       * \param fn The function to be applied to each non-trivial (qarg, PauliOp) pair  
       */
      template<typename Fn>
      void foreach(Fn&& fn) const {
        static_assert(std::is_invocable_r_v<void, Fn, std::pair<qarg, PauliOp> const&>);
        for (auto& p : pauli_) fn(p);
      }
      /**@}*/

      /** @name Operators */
      /**@{*/
      /** \brief Scale-assign a Pauli by a (Pauli) phase */
      Pauli& operator*=(const IPhase& phase) {
        phase_ *= phase;
        return *this;
      }
      /** \brief Scale a Pauli by a (Pauli) phase */
      Pauli operator*(const IPhase& phase) const {
        auto tmp_(*this);
        tmp_ *= phase;
        return tmp_;
      }

      /** \brief Multiply-assign a Pauli by a Pauli */
      Pauli& operator*=(const Pauli& P) {
        phase_ *= P.phase_;
        for (auto& [q, p] : P.pauli_) {
          phase_ *= normal_phase(pauli_[q], p);
          pauli_[q] *= p;
        }
        return *this;
      }
      /** \brief Multiply a Pauli by a Pauli */
      Pauli operator*(const Pauli& P) const {
        auto tmp_(*this);
        tmp_ *= P;
        return tmp_;
      }

      /** \brief Negate the Pauli's phase */
      Pauli operator-() const {
        return (*this) * IPhase::two;
      }

      /** \brief Equality between Paulis */
      bool operator==(const Pauli& P) const {
        if (phase_ != P.phase_) return false;
      
        for (auto& [q, p] : P.pauli_) {
          auto it = pauli_.find(q);
          auto tmp = it == pauli_.end() ? PauliOp::i : it->second;

          if (tmp != p) return false;
        }

        for (auto& [q, p] : pauli_) {
          auto it = P.pauli_.find(q);
          auto tmp = it == P.pauli_.end() ? PauliOp::i : it->second;

          if (tmp != p) return false;
        }

        return true;
      }
      /** \brief Inequality between Paulis */
      bool operator!=(const Pauli& P) const {
        return !(*this == P);
      }
      /**@}*/

      /** @name Queries */
      /**@{*/
      /** 
       * \brief Check whether it commutes with another Pauli 
       * \param P A constant reference to a Pauli
       * \return True if it commutes with P
       */
      bool commutes_with(const Pauli& P) const {
        uint32_t tot_anti = 0;
      
        for (auto& [q, p] : P.pauli_) {
          auto it = pauli_.find(q);
          if (it != pauli_.end() && !paulis_commute(it->second, p)) tot_anti++;
        }

        return (tot_anti % 2) == 0;
      }

      /** 
       * \brief Check whether it acts trivially on a qubit
       * \param q Const reference to a quantum argument
       * \return True if the Pauli acts trivially on q
       */
      bool trivial_on(const qarg& q) const {
        auto it = pauli_.find(q);
        if (it == pauli_.end() || it->second == PauliOp::i) return true;
        return false;
      }

      /** 
       * \brief Check if it is strictly a Z axis rotation
       * \return True if the Pauli is I or Z on all qubits
       */
      bool is_z() const {
        for (auto& [q, p] : pauli_) {
          if ((p != PauliOp::i) && (p != PauliOp::z)) return false;
        }
        return true;
      }
      /**@}*/

      /** @name Printing */
      /**@{*/
      /** \brief Pretty printing of Paulis */
      std::ostream& print(std::ostream& os) const {
        os << phase_;
        for (auto& [q, p] : pauli_) {
          os << p << "(" << q << ")";
        }
        return os;
      }
      /**@}*/

    private:
      std::unordered_map<qarg, PauliOp> pauli_; ///< the non-trivial single-qubit Paulis
      IPhase phase_ = IPhase::zero;             ///< the phase of the Pauli

    };

    /** \brief Extraction operation overload for Paulis */
    friend std::ostream& operator<<(std::ostream& os, const Pauli& P) { return P.print(os); }


    /** 
     * \classs staq::gates::ChannelRepr::Clifford
     * \brief Class representing an n-qubit Clifford operator as the normalizer of the Pauli group
     *
     * Cliffords are represented via a sparse mapping from a (non-minimal) set of generators of 
     * the n-qubit Pauli group to an n-qubit Pauli operator, defined by permutation of the Pauli
     * group under conjugation -- i.e. 
     *    \f$C(P_1P_2\cdots P_n)C^\dagger = CP_1C^\dagger CP_2C^\dagger\cdots CP_nC^\dagger\f$
     * No mapping means the operator acts trivially on that generator.
     */
    class Clifford {
    public:
      /** @name Constructors */
      /**@{*/
      /** \brief Empty constructor */
      Clifford() {}
      /** \brief Construct a Clifford from a mapping from single-qubit Paulis to multi-qubit Paulis */
      Clifford(std::map<std::pair<qarg, PauliOp>, Pauli> perm) : perm_(perm) {}
      /**@}*/

      /** @name Smart constructors */
      /**@{*/
      /** \brief Construct an \f$H\f$ gate */
      static Clifford h(const qarg& q) {
        return Clifford(
                        { { std::make_pair(q, PauliOp::x), Pauli::z(q) },
                          { std::make_pair(q, PauliOp::z), Pauli::x(q) },
                          { std::make_pair(q, PauliOp::y), -(Pauli::y(q)) } });
      }
      /** \brief Construct an \f$S\f$ gate */
      static Clifford s(const qarg& q) {
        return Clifford(
                        { { std::make_pair(q, PauliOp::x), Pauli::y(q) },
                          { std::make_pair(q, PauliOp::y), -(Pauli::x(q)) } });
      }
      /** \brief Construct an \f$S^\dagger\f$ gate */
      static Clifford sdg(const qarg& q) {
        return Clifford(
                        { { std::make_pair(q, PauliOp::x), -(Pauli::y(q)) },
                          { std::make_pair(q, PauliOp::y), Pauli::x(q) } });
      }
      /** \brief Construct a \f$CNOT\f$ gate */
      static Clifford cnot(const qarg& q1, const qarg& q2) {
        return Clifford(
                        { { std::make_pair(q1, PauliOp::x), Pauli::x(q1) * Pauli::x(q2) },
                          { std::make_pair(q2, PauliOp::z), Pauli::z(q1) * Pauli::z(q2) },
                          { std::make_pair(q1, PauliOp::y), Pauli::y(q1) * Pauli::x(q2) },
                          { std::make_pair(q2, PauliOp::y), Pauli::z(q1) * Pauli::y(q2) } });
      }
      /** \brief Construct a (Clifford) \f$X\f$ gate */
      static Clifford x(const qarg& q) {
        return Clifford(
                        { { std::make_pair(q, PauliOp::z), -(Pauli::z(q)) },
                          { std::make_pair(q, PauliOp::y), -(Pauli::y(q)) } });
      }
      /** \brief Construct a (Clifford) \f$Z\f$ gate */
      static Clifford z(const qarg& q) {
        return Clifford(
                        { { std::make_pair(q, PauliOp::x), -(Pauli::x(q)) },
                          { std::make_pair(q, PauliOp::y), -(Pauli::y(q)) } });
      }
      /** \brief Construct a (Clifford) \f$Y\f$ gate */
      static Clifford y(const qarg& q) {
        return Clifford(
                        { { std::make_pair(q, PauliOp::x), -(Pauli::x(q)) },
                          { std::make_pair(q, PauliOp::z), -(Pauli::z(q)) } });
      }
      /**@}*/

      /** @name Operators */
      /**@{*/
      /**
       * \brief Conjugate a Pauli
       * \param P Const reference to a Pauli
       * \return Pauli that is equal to \f$CPC^\dagger\f$
       */
      Pauli conjugate(const Pauli& P) const {
        Pauli ret;
        ret *= P.phase();
      
        P.foreach([&ret, this](auto& p) {
                    auto it = this->perm_.find(p);
                    if (it == perm_.end()) {
                      ret *= Pauli(p);
                    } else {
                      ret *= it->second;
                    }
                  });

        return ret;
      }

      /** \brief Multiply-assign by a Clifford */
      Clifford& operator*=(const Clifford& C) {
        *this = *this * C;
        return *this;
      }
      /** \brief Multiply by a Clifford */
      Clifford operator*(const Clifford& C) {
        Clifford ret(*this);
        for (auto& [pauli_in, pauli_out] : C.perm_) {
          ret.perm_[pauli_in] = conjugate(pauli_out);
        }
        return ret;
      }
      /**@}*/

      /** @name Printing */
      /**@{*/
      /** \brief Pretty printer */
      std::ostream& print(std::ostream& os) const {
        os << "{ ";
        for (auto& [pauli_in, pauli_out] : perm_) {
          os << Pauli(pauli_in) << " --> " << pauli_out << ", ";
        }
        os << "}";

        return os;
      }
      /**@}*/

    private:
      /** The permutation as a map from single-qubit Paulis to multi-qubit Paulis */
      std::map<std::pair<qarg, PauliOp>, Pauli> perm_;

    };

    /** \brief Extraction operator overload for Cliffords */
    friend std::ostream& operator<<(std::ostream& os, const Clifford& P) { return P.print(os); }


    /** 
     * \class staq::gates::ChannelRepr::Uninterp
     * \brief Class storing an uninterpreted operation on some set of qubits 
     *
     * Uninterpreted gates are gates that do not correspond to a Pauli,
     * Clifford, or Rotation, or the representation as one is otherwise
     * unknown. They can be thought of as a black box on some set of qubits.
     * Operations can be otherwise commuted around them so long as they act
     * on disjoint qubits
     */
    class Uninterp {
    public:
      /** \brief Construct an uninterpreted gate on some list of qubits */
      Uninterp(std::vector<qarg> qubits) : qubits_(qubits) {}

      /** 
       * \brief Apply a function to each qubit
       *
       * \tparam Fn The type of the function to be applied. 
       *            Must be invocable on a qarg
       * \param fn The function to be applied to each qarg
       */
      template<typename Fn>
      void foreach_qubit(Fn&& fn) const {
        static_assert(std::is_invocable_r_v<void, Fn, qarg const&>);
        for (auto& q : qubits_) fn(q);
      }

      /** \brief Pretty printer */
      std::ostream& print(std::ostream& os) const {
        os << "U(";
        for (auto& q : qubits_) os << q << ",";
        os << ")";

        return os;
      }

    private:
      std::vector<qarg> qubits_; ///< A list of qubits the gate acts non-trivially on
    };
    
    /** \brief Extraction operator overload for Uninterpreted gates */
    friend std::ostream& operator<<(std::ostream& os, const Uninterp& P) { return P.print(os); }

    /** 
     * \class staq::gates::ChannelRepr::Rotation
     * \brief Class storing a rotation of some angle around a pauli
     *
     * A rotation with angle \f$\theta\f$ and Pauli \f$P\f$ represents the unitary
     *     \f$\frac{1 + e^{i\theta}}{2} I + \frac{1 - e^{i\theta}}{2} P\f$
     */
    class Rotation {
    public:
      /** @name Constructors */
      /**@{*/
      /** \brief Empty constructor */
      Rotation() : theta_(utils::angles::zero) {}
      /** \brief Construct a rotation from an angle and Pauli */
      Rotation(utils::Angle theta, Pauli pauli) : theta_(theta), pauli_(pauli) {}
      /**@}*/

      /** @name Smart constructors */
      /**@{*/
      /** \brief Construct a \f$T\f$ gate */
      static Rotation t(const qarg& q) {
        return Rotation(utils::angles::pi_quarter, Pauli::z(q));
      }
      /** \brief Construct a \f$T^\dagger\f$ gate */
      static Rotation tdg(const qarg& q) {
        return Rotation(-utils::angles::pi_quarter, Pauli::z(q));
      }
      /** \brief Construct an \f$R_Z(\theta)\f$ gate */
      static Rotation rz(utils::Angle theta, const qarg& q) {
        return Rotation(theta, Pauli::z(q));
      }
      /** \brief Construct an \f$R_X(\theta)\f$ gate */
      static Rotation rx(utils::Angle theta, const qarg& q) {
        return Rotation(theta, Pauli::x(q));
      }
      /** \brief Construct an \f$R_Y(\theta)\f$ gate */
      static Rotation ry(utils::Angle theta, const qarg& q) {
        return Rotation(theta, Pauli::y(q));
      }
      /**@}*/


      /** @name Accessors */
      /**@{*/
      /** \brief Get the angle of rotation */
      utils::Angle rotation_angle() { return theta_; }
      /**@}*/

      /** @name Operators */
      /**@{*/
      /** 
       * \brief Commutes a rotation through a Clifford on the left
       * \param C Const reference to a Clifford gate
       * \return A rotation \f$R(\theta, P')\f$ such that 
       *              \f$CR(\theta, P) = R(\theta, P')C\f$
       */
      Rotation commute_left(const Clifford& C) const {
        auto tmp = Rotation(*this);
        tmp.pauli_ = C.conjugate(tmp.pauli_);
        return tmp;
      }

      /** \brief Equality between Rotation gates */
      bool operator==(const Rotation& R) const {
        return (theta_ == R.theta_) && (pauli_ == R.pauli_);
      }
      /** \brief Inequality between Rotation gates */
      bool operator!=(const Rotation& R) const {
        return !(*this == R);
      }

      /** \brief Checks whether the rotation commutes with another rotation */
      bool commutes_with(const Rotation& R) const {
        return pauli_.commutes_with(R.pauli_);
      }

      /** \brief Checks whether the rotation commutes with an uninterpreted gate */
      bool commutes_with(const Uninterp& U) const {
        auto tmp = true;

        U.foreach_qubit([&tmp, this](const qarg& q) {
                          tmp &= pauli_.trivial_on(q);
                        });

        return tmp;
      }

      /** 
       * \brief Attempts to merge with another rotation
       * \param R Const reference to a rotation gate
       * \return A phase \f$\phi\f$ and rotation \f$R'\f$ such that \f$\phi R'\f$
       *         is equal to the product of the object and R
       */
      std::optional<std::pair<utils::Angle, Rotation> > try_merge(const Rotation& R) const {
        if (pauli_ == R.pauli_) {
          auto phase = utils::angles::zero;
          auto rotation = Rotation(theta_ + R.theta_, pauli_);
          return std::make_optional(std::make_pair(phase, rotation));
        } else if (pauli_ == -(R.pauli_)) {
          auto phase = R.theta_;
          auto rotation = Rotation(theta_ + -R.theta_, pauli_);
          return std::make_optional(std::make_pair(phase, rotation));
        } else {
          return std::nullopt;
        }
      }

      /** \brief Checks whether the rotation is a Z axis rotation */
      bool is_z_rotation() const {
        return pauli_.is_z();
      }
      /**@}*/

      /* Printing */
      /** @name Printing */
      /**@{*/
      /** \brief Pretty printer */
      std::ostream& print(std::ostream& os) const {
        os << "R(" << theta_ << ", " << pauli_ << ")";

        return os;
      }
      /**@}*/

    private:
      utils::Angle theta_; ///< The angle of rotation
      Pauli pauli_;        ///< The Pauli rotated on

    };

    /** \brief Extraction operator overload for Rotation gates */
    friend std::ostream& operator<<(std::ostream& os, const Rotation& P) { return P.print(os); }

  };

}
}
