/*-------------------------------------------------------------------------------------------------
  | This file is distributed under the MIT License.
  | See accompanying file /LICENSE for details.
  | Author(s): Matthew Amy
  *------------------------------------------------------------------------------------------------*/

#include <unordered_map>
#include <map>
#include <set>
#include <variant>
#include <iostream>

#include <tweedledum/utils/angle.hpp>

namespace synthewareQ {
namespace channel {

  /*! \brief Optimizations based on the channel representation of Clifford + single qubit gates */

  using id = std::string;


  enum class pauli_gate  : unsigned short { i = 0, x = 1, z = 2, y = 3 };
  enum class pauli_phase : unsigned short { zero = 0, one = 1, two = 2, three = 3 };

  inline pauli_gate operator*(const pauli_gate& p, const pauli_gate& q) {
    return static_cast<pauli_gate>(static_cast<unsigned short>(p) ^ static_cast<unsigned short>(q));
  }
  inline pauli_gate& operator*=(pauli_gate& p, const pauli_gate& q) {
    p = p * q;
    return p;
  }
  std::ostream& operator<<(std::ostream& os, const pauli_gate& p) {
    switch(p) {
    case pauli_gate::i: os << "I"; break;
    case pauli_gate::x: os << "X"; break;
    case pauli_gate::z: os << "Z"; break;
    case pauli_gate::y: os << "Y"; break;
    }

    return os;
  }

  inline pauli_phase operator*(const pauli_phase& a, const pauli_phase& b) {
    return static_cast<pauli_phase>((static_cast<unsigned short>(a) + static_cast<unsigned short>(b)) % 4);
  }
  inline pauli_phase operator*=(pauli_phase& a, const pauli_phase& b) {
    a = a * b;
    return a;
  }
  std::ostream& operator<<(std::ostream& os, const pauli_phase& p) {
    switch(p) {
    case pauli_phase::zero: os << ""; break;
    case pauli_phase::one: os << "i"; break;
    case pauli_phase::two: os << "-"; break;
    case pauli_phase::three: os << "-i"; break;
    }

    return os;
  }
  inline pauli_phase normal_phase(const pauli_gate& p, const pauli_gate& q) {
    switch(static_cast<unsigned short>(p) | (static_cast<unsigned short>(q) << 2)) {
    case 0:  // II
    case 1:  // XI
    case 2:  // ZI
    case 3:  // YI
    case 4:  // IX
    case 5:  // XX
    case 8:  // IZ
    case 10: // ZZ
    case 12: // IY
    case 15: // YY
      return pauli_phase::zero;
    case 6:  // ZX
    case 11: // YZ
    case 13: // XY
      return pauli_phase::one;
    case 7:  // YX
    case 9:  // XZ
    case 14: // ZY
      return pauli_phase::three;
    }
  }



  /*! \brief Class representing an n-qubit pauli operator */
  class pauli_op {
  public:
    pauli_op() {}
    pauli_op(std::pair<id, pauli_gate> gate) { pauli_[gate.first] = gate.second; }
    pauli_op(std::unordered_map<id, pauli_gate> pauli) : pauli_(pauli) {}

    // Convenience constructors
    static pauli_op i_gate(id q) { return pauli_op({{ q, pauli_gate::i }}); }
    static pauli_op x_gate(id q) { return pauli_op({{ q, pauli_gate::x }}); }
    static pauli_op z_gate(id q) { return pauli_op({{ q, pauli_gate::z }}); }
    static pauli_op y_gate(id q) { return pauli_op({{ q, pauli_gate::y }}); }

    pauli_op& operator*=(const pauli_phase& phase) {
      phase_ *= phase;
      return *this;
    }
    pauli_op operator*(const pauli_phase& phase) {
      auto tmp_(*this);
      tmp_ *= phase;
      return tmp_;
    }
    pauli_op& operator*=(const pauli_op& P) {
      phase_ *= P.phase_;
      for (auto& [q, p] : P.pauli_) {
        phase_ *= normal_phase(pauli_[q], p);
        pauli_[q] *= p;
      }
      return *this;
    }
    pauli_op operator*(const pauli_op& P) {
      auto tmp_(*this);
      tmp_ *= P;
      return tmp_;
    }


    std::ostream& print(std::ostream& os) const {
      os << phase_;
      for (auto& [q, p] : pauli_) {
        os << p << "(" << q << ")";
      }
      return os;
    }

    pauli_phase phase() const { return phase_; }
    template<typename Fn>
    void for_each(Fn&& fn) const {
      static_assert(std::is_invocable_r_v<void, Fn, std::pair<id, pauli_gate> const&>);
      for (auto& p : pauli_) fn(p);
    }

  private:
    std::unordered_map<id, pauli_gate> pauli_;
    pauli_phase phase_ = pauli_phase::zero;

  };

  std::ostream& operator<<(std::ostream& os, const pauli_op& P) { return P.print(os); }


  /*! \brief Class representing an n-qubit Clifford operator as the normalizer of the Pauli group
   *
   *  Cliffords are represented via a sparse mapping from a (non-minimal) set of generators of 
   *  the n-qubit Pauli group to an n-qubit Pauli operator, defined by permutation of the Pauli
   *  group under conjugation -- i.e. CPC^* = CP_1C^*CP_2C^*... 
   *  
   *  Note: no mapping means the operator acts trivially on that generator */
  class clifford_op {
  public:
    static clifford_op h_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::z_gate(q) },
          { std::make_pair(q, pauli_gate::z), pauli_op::x_gate(q) },
          { std::make_pair(q, pauli_gate::y), pauli_op::y_gate(q)*pauli_phase::two } });
    }
    static clifford_op s_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::y_gate(q) },
          { std::make_pair(q, pauli_gate::y), pauli_op::x_gate(q)*pauli_phase::two } });
    }
    static clifford_op sdg_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::y_gate(q)*pauli_phase::two },
          { std::make_pair(q, pauli_gate::y), pauli_op::x_gate(q) } });
    }
    static clifford_op cnot_gate(id q1, id q2) {
      return clifford_op(
        { { std::make_pair(q1, pauli_gate::x), pauli_op::x_gate(q1) * pauli_op::x_gate(q2) },
          { std::make_pair(q2, pauli_gate::z), pauli_op::z_gate(q1) * pauli_op::z_gate(q2) },
          { std::make_pair(q1, pauli_gate::y), pauli_op::y_gate(q1) * pauli_op::x_gate(q2) },
          { std::make_pair(q2, pauli_gate::y), pauli_op::z_gate(q1) * pauli_op::y_gate(q2) } });
    }
    // Paulis
    static clifford_op x_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::z), pauli_op::z_gate(q)*pauli_phase::two },
          { std::make_pair(q, pauli_gate::y), pauli_op::y_gate(q)*pauli_phase::two } });
    }
    static clifford_op z_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::x_gate(q)*pauli_phase::two },
          { std::make_pair(q, pauli_gate::y), pauli_op::y_gate(q)*pauli_phase::two } });
    }
    static clifford_op y_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::x_gate(q)*pauli_phase::two },
          { std::make_pair(q, pauli_gate::z), pauli_op::z_gate(q)*pauli_phase::two } });
    }

    pauli_op conjugate(const pauli_op& P) {
      pauli_op ret;
      ret *= P.phase();
      
      P.for_each([&ret, this](auto& p) {
          if (this->perm_.find(p) == perm_.end()) {
            ret *= pauli_op(p);
          } else {
            ret *= this->perm_[p];
          }
        });

      return ret;
    }

    clifford_op& operator*=(const clifford_op& C) {
      *this = *this * C;
      return *this;
    }
    clifford_op operator*(const clifford_op& C) {
      clifford_op ret(*this);
      for (auto& [pauli_in, pauli_out] : C.perm_) {
        ret.perm_[pauli_in] = conjugate(pauli_out);
      }
      return ret;
    }

    std::ostream& print(std::ostream& os) const {
      os << "{ ";
      for (auto& [pauli_in, pauli_out] : perm_) {
        os << pauli_op(pauli_in) << " --> " << pauli_out << ", ";
      }
      os << "}\n";

      return os;
    }

  private:
    clifford_op(std::map<std::pair<id, pauli_gate>, pauli_op> perm) : perm_(perm) {}

    std::map<std::pair<id, pauli_gate>, pauli_op> perm_;
  };

  std::ostream& operator<<(std::ostream& os, const clifford_op& P) { return P.print(os); }


  /*! \brief Class storing a rotation of some angle around a pauli
   *
   *  (1 + e^i\theta)/2 I + (1 - e^i\theta) P
   */
  class rotation_op {
  public:

  private:
    tweedledum::angle theta;
    pauli_op pauli_;

  };


  /*! \brief Class storing an uninterpreted operation on some set of qubits */
  class uninterp_op {
  public:

  private:
    std::set<id> qubits_;
  };
    
  typedef std::variant<rotation_op, clifford_op, uninterp_op> op;

}
}
