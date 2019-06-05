/*
 * This file is part of synthewareQ.
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

/**
 * \file circuits/channel.hpp
 * \brief Gates in the channel representation
 */
#pragma once

#include "utils/angle.hpp"

#include <unordered_map>
#include <map>
#include <vector>
#include <variant>
#include <iostream>

namespace synthewareQ {
namespace gates {
  /*! \brief Utilities for the channel representation of Clifford + single qubit gates */

  template<typename qarg>
  struct ChannelRepr {

  /* The single qubit Pauli group and operations on it */
  enum class PauliOp  : unsigned short { i = 0, x = 1, z = 2, y = 3 };
  enum class IPhase : unsigned short { zero = 0, one = 1, two = 2, three = 3 };


  friend inline PauliOp operator*(const PauliOp& p, const PauliOp& q) {
    return static_cast<PauliOp>(static_cast<unsigned short>(p) ^ static_cast<unsigned short>(q));
  }
  friend inline PauliOp& operator*=(PauliOp& p, const PauliOp& q) {
    p = p * q;
    return p;
  }
  friend std::ostream& operator<<(std::ostream& os, const PauliOp& p) {
    switch(p) {
    case PauliOp::i: os << "I"; break;
    case PauliOp::x: os << "X"; break;
    case PauliOp::z: os << "Z"; break;
    case PauliOp::y: os << "Y"; break;
    }

    return os;
  }

  friend inline IPhase operator*(const IPhase& a, const IPhase& b) {
    return static_cast<IPhase>((static_cast<unsigned short>(a) + static_cast<unsigned short>(b)) % 4);
  }
  friend inline IPhase operator*=(IPhase& a, const IPhase& b) {
    a = a * b;
    return a;
  }
  friend std::ostream& operator<<(std::ostream& os, const IPhase& p) {
    switch(p) {
    case IPhase::zero: os << ""; break;
    case IPhase::one: os << "i"; break;
    case IPhase::two: os << "-"; break;
    case IPhase::three: os << "-i"; break;
    }

    return os;
  }

  inline static IPhase normal_phase(const PauliOp& p, const PauliOp& q) {
    static IPhase phase_mult_table[16] = {
      IPhase::zero, // II
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
      IPhase::zero // YY
    };
    
    auto idx = (static_cast<unsigned short>(p) | (static_cast<unsigned short>(q) << 2));
    return phase_mult_table[idx % 16];
  }

  inline static bool paulis_commute (const PauliOp& p, const PauliOp& q) {
    static bool commute_table[16] = {
      true, // II
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
      true // YY
    };
    
    auto idx = (static_cast<unsigned short>(p) | (static_cast<unsigned short>(q) << 2));
    return commute_table[idx % 16];
  }

  /*! \brief Class representing an n-qubit pauli operator */
  class Pauli {
  public:
    Pauli() {}
    Pauli(std::pair<qarg, PauliOp> gate) { pauli_[gate.first] = gate.second; }
    Pauli(std::unordered_map<qarg, PauliOp> pauli) : pauli_(pauli) {}
    static Pauli i(const qarg& q) { return Pauli({{ q, PauliOp::i }}); }
    static Pauli x(const qarg& q) { return Pauli({{ q, PauliOp::x }}); }
    static Pauli z(const qarg& q) { return Pauli({{ q, PauliOp::z }}); }
    static Pauli y(const qarg& q) { return Pauli({{ q, PauliOp::y }}); }

    /* Accessors */
    IPhase phase() const { return phase_; }

    template<typename Fn>
    void foreach(Fn&& fn) const {
      static_assert(std::is_invocable_r_v<void, Fn, std::pair<qarg, PauliOp> const&>);
      for (auto& p : pauli_) fn(p);
    }

    /* Operators */
    Pauli& operator*=(const IPhase& phase) {
      phase_ *= phase;
      return *this;
    }
    Pauli operator*(const IPhase& phase) const {
      auto tmp_(*this);
      tmp_ *= phase;
      return tmp_;
    }

    Pauli& operator*=(const Pauli& P) {
      phase_ *= P.phase_;
      for (auto& [q, p] : P.pauli_) {
        phase_ *= normal_phase(pauli_[q], p);
        pauli_[q] *= p;
      }
      return *this;
    }
    Pauli operator*(const Pauli& P) const {
      auto tmp_(*this);
      tmp_ *= P;
      return tmp_;
    }

    Pauli operator-() const {
      return (*this) * IPhase::two;
    }

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

    bool commutes_with(const Pauli& P) const {
      uint32_t tot_anti = 0;
      
      for (auto& [q, p] : P.pauli_) {
        auto it = pauli_.find(q);
        if (it != pauli_.end() && !paulis_commute(it->second, p)) tot_anti++;
      }

      return (tot_anti % 2) == 0;
    }

    bool trivial_on(const qarg& q) const {
      auto it = pauli_.find(q);
      if (it == pauli_.end() || it->second == PauliOp::i) return true;
      return false;
    }

    bool is_z() const {
      for (auto& [q, p] : pauli_) {
        if ((p != PauliOp::i) && (p != PauliOp::z)) return false;
      }
      return true;
    }

    /* Printing */
    std::ostream& print(std::ostream& os) const {
      os << phase_;
      for (auto& [q, p] : pauli_) {
        os << p << "(" << q << ")";
      }
      return os;
    }

  private:
    std::unordered_map<qarg, PauliOp> pauli_;
    IPhase phase_ = IPhase::zero;

  };

  friend std::ostream& operator<<(std::ostream& os, const Pauli& P) { return P.print(os); }


  /*! \brief Class representing an n-qubit Clifford operator as the normalizer of the Pauli group
   *
   *  Cliffords are represented via a sparse mapping from a (non-minimal) set of generators of 
   *  the n-qubit Pauli group to an n-qubit Pauli operator, defined by permutation of the Pauli
   *  group under conjugation -- i.e. CPC^* = CP_1C^*CP_2C^*... 
   *  
   *  Note: no mapping means the operator acts trivially on that generator */
  class Clifford {
  public:
    Clifford() {}
    Clifford(std::map<std::pair<qarg, PauliOp>, Pauli> perm) : perm_(perm) {}
    static Clifford h(const qarg& q) {
      return Clifford(
        { { std::make_pair(q, PauliOp::x), Pauli::z(q) },
          { std::make_pair(q, PauliOp::z), Pauli::x(q) },
          { std::make_pair(q, PauliOp::y), -(Pauli::y(q)) } });
    }
    static Clifford s(const qarg& q) {
      return Clifford(
        { { std::make_pair(q, PauliOp::x), Pauli::y(q) },
          { std::make_pair(q, PauliOp::y), -(Pauli::x(q)) } });
    }
    static Clifford sdg(const qarg& q) {
      return Clifford(
        { { std::make_pair(q, PauliOp::x), -(Pauli::y(q)) },
          { std::make_pair(q, PauliOp::y), Pauli::x(q) } });
    }
    static Clifford cnot(const qarg& q1, const qarg& q2) {
      return Clifford(
        { { std::make_pair(q1, PauliOp::x), Pauli::x(q1) * Pauli::x(q2) },
          { std::make_pair(q2, PauliOp::z), Pauli::z(q1) * Pauli::z(q2) },
          { std::make_pair(q1, PauliOp::y), Pauli::y(q1) * Pauli::x(q2) },
          { std::make_pair(q2, PauliOp::y), Pauli::z(q1) * Pauli::y(q2) } });
    }
    static Clifford x(const qarg& q) {
      return Clifford(
        { { std::make_pair(q, PauliOp::z), -(Pauli::z(q)) },
          { std::make_pair(q, PauliOp::y), -(Pauli::y(q)) } });
    }
    static Clifford z(const qarg& q) {
      return Clifford(
        { { std::make_pair(q, PauliOp::x), -(Pauli::x(q)) },
          { std::make_pair(q, PauliOp::y), -(Pauli::y(q)) } });
    }
    static Clifford y(const qarg& q) {
      return Clifford(
        { { std::make_pair(q, PauliOp::x), -(Pauli::x(q)) },
          { std::make_pair(q, PauliOp::z), -(Pauli::z(q)) } });
    }

    /* Operators */
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

    Clifford& operator*=(const Clifford& C) {
      *this = *this * C;
      return *this;
    }
    Clifford operator*(const Clifford& C) {
      Clifford ret(*this);
      for (auto& [pauli_in, pauli_out] : C.perm_) {
        ret.perm_[pauli_in] = conjugate(pauli_out);
      }
      return ret;
    }

    /* Printing */
    std::ostream& print(std::ostream& os) const {
      os << "{ ";
      for (auto& [pauli_in, pauli_out] : perm_) {
        os << Pauli(pauli_in) << " --> " << pauli_out << ", ";
      }
      os << "}";

      return os;
    }

  private:
    std::map<std::pair<qarg, PauliOp>, Pauli> perm_;

  };

  friend std::ostream& operator<<(std::ostream& os, const Clifford& P) { return P.print(os); }


  /*! \brief Class storing an uninterpreted operation on some set of qubits */
  class Uninterp {
  public:
    Uninterp(std::vector<qarg> qubits) : qubits_(qubits) {}

    template<typename Fn>
    void foreach_qubit(Fn&& fn) const {
      static_assert(std::is_invocable_r_v<void, Fn, qarg const&>);
      for (auto& q : qubits_) fn(q);
    }

    /* Printing */
    std::ostream& print(std::ostream& os) const {
      os << "U(";
      for (auto& q : qubits_) os << q << ",";
      os << ")";

      return os;
    }

  private:
    std::vector<qarg> qubits_;
  };
    
  friend std::ostream& operator<<(std::ostream& os, const Uninterp& P) { return P.print(os); }


  /*! \brief Class storing a rotation of some angle around a pauli
   *
   *  (1 + e^i\theta)/2 I + (1 - e^i\theta) P
   */
  class Rotation {
  public:
    Rotation() : theta_(utils::angles::zero) {}
    Rotation(utils::Angle theta, Pauli pauli) : theta_(theta), pauli_(pauli) {}
    static Rotation t(const qarg& q) {
      return Rotation(utils::angles::pi_quarter, Pauli::z(q));
    }
    static Rotation tdg(const qarg& q) {
      return Rotation(-utils::angles::pi_quarter, Pauli::z(q));
    }
    static Rotation rz(utils::Angle theta, const qarg& q) {
      return Rotation(theta, Pauli::z(q));
    }
    static Rotation rx(utils::Angle theta, const qarg& q) {
      return Rotation(theta, Pauli::x(q));
    }
    static Rotation ry(utils::Angle theta, const qarg& q) {
      return Rotation(theta, Pauli::y(q));
    }


    /* Accessors */
    utils::Angle rotation_angle() { return theta_; }

    /* Operators */

    // CR(theta, P) == R(theta, P')C
    Rotation commute_left(const Clifford& C) const {
      auto tmp = Rotation(*this);
      tmp.pauli_ = C.conjugate(tmp.pauli_);
      return tmp;
    }

    bool operator==(const Rotation& R) const {
      return (theta_ == R.theta_) && (pauli_ == R.pauli_);
    }

    bool commutes_with(const Rotation& R) const {
      return pauli_.commutes_with(R.pauli_);
    }

    bool commutes_with(const Uninterp& U) const {
      auto tmp = true;

      U.foreach_qubit([&tmp, this](const qarg& q) {
          tmp &= pauli_.trivial_on(q);
        });

      return tmp;
    }

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

    bool is_z_rotation() const {
      return pauli_.is_z();
    }

    /* Printing */
    std::ostream& print(std::ostream& os) const {
      os << "R(" << theta_ << ", " << pauli_ << ")";

      return os;
    }

  private:
    utils::Angle theta_;
    Pauli pauli_;

  };

  friend std::ostream& operator<<(std::ostream& os, const Rotation& P) { return P.print(os); }


};

}
}
