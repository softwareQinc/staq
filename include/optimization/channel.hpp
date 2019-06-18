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

  namespace td = tweedledum;

  /*! \brief Optimizations based on the channel representation of Clifford + single qubit gates */

  using id = std::string;


  /* The single qubit Pauli group and operations on it */
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
    static pauli_phase phase_mult_table[16] = {
      pauli_phase::zero, // II
      pauli_phase::zero, // XI
      pauli_phase::zero, // ZI
      pauli_phase::zero, // YI
      pauli_phase::zero, // IX
      pauli_phase::zero, // XX
      pauli_phase::one, // ZX
      pauli_phase::three, // YX
      pauli_phase::zero, // IZ
      pauli_phase::three, // XZ
      pauli_phase::zero, // ZZ
      pauli_phase::one, // YZ
      pauli_phase::zero, // IY
      pauli_phase::one, // XY
      pauli_phase::three, // ZY
      pauli_phase::zero // YY
    };
    
    auto idx = (static_cast<unsigned short>(p) | (static_cast<unsigned short>(q) << 2));
    return phase_mult_table[idx % 16];
  }

  inline bool paulis_commute (const pauli_gate& p, const pauli_gate& q) {
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
  class pauli_op {
  public:
    pauli_op() {}
    pauli_op(std::pair<id, pauli_gate> gate) { pauli_[gate.first] = gate.second; }
    pauli_op(std::unordered_map<id, pauli_gate> pauli) : pauli_(pauli) {}

    /* Gate constructors */
    static pauli_op i_gate(id q) { return pauli_op({{ q, pauli_gate::i }}); }
    static pauli_op x_gate(id q) { return pauli_op({{ q, pauli_gate::x }}); }
    static pauli_op z_gate(id q) { return pauli_op({{ q, pauli_gate::z }}); }
    static pauli_op y_gate(id q) { return pauli_op({{ q, pauli_gate::y }}); }

    /* Accessors */
    pauli_phase phase() const { return phase_; }

    template<typename Fn>
    void for_each(Fn&& fn) const {
      static_assert(std::is_invocable_r_v<void, Fn, std::pair<id, pauli_gate> const&>);
      for (auto& p : pauli_) fn(p);
    }

    /* Operators */
    pauli_op& operator*=(const pauli_phase& phase) {
      phase_ *= phase;
      return *this;
    }
    pauli_op operator*(const pauli_phase& phase) const {
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
    pauli_op operator*(const pauli_op& P) const {
      auto tmp_(*this);
      tmp_ *= P;
      return tmp_;
    }

    pauli_op operator-() const {
      return (*this) * pauli_phase::two;
    }

    bool operator==(const pauli_op& P) const {
      if (phase_ != P.phase_) return false;
      
      for (auto& [q, p] : P.pauli_) {
        auto it = pauli_.find(q);
        auto tmp = it == pauli_.end() ? pauli_gate::i : it->second;

        if (tmp != p) return false;
      }

      for (auto& [q, p] : pauli_) {
        auto it = P.pauli_.find(q);
        auto tmp = it == P.pauli_.end() ? pauli_gate::i : it->second;

        if (tmp != p) return false;
      }

      return true;
    }

    bool commutes_with(const pauli_op& P) const {
      uint32_t tot_anti = 0;
      
      for (auto& [q, p] : P.pauli_) {
        auto it = pauli_.find(q);
        if (it != pauli_.end() && !paulis_commute(it->second, p)) tot_anti++;
      }

      return (tot_anti % 2) == 0;
    }

    bool trivial_on(const id q) const {
      auto it = pauli_.find(q);
      if (it == pauli_.end() || it->second == pauli_gate::i) return true;
      return false;
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
    /* Gate constructors */
    static clifford_op h_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::z_gate(q) },
          { std::make_pair(q, pauli_gate::z), pauli_op::x_gate(q) },
          { std::make_pair(q, pauli_gate::y), -(pauli_op::y_gate(q)) } });
    }
    static clifford_op s_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), pauli_op::y_gate(q) },
          { std::make_pair(q, pauli_gate::y), -(pauli_op::x_gate(q)) } });
    }
    static clifford_op sdg_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), -(pauli_op::y_gate(q)) },
          { std::make_pair(q, pauli_gate::y), pauli_op::x_gate(q) } });
    }
    static clifford_op cnot_gate(id q1, id q2) {
      return clifford_op(
        { { std::make_pair(q1, pauli_gate::x), pauli_op::x_gate(q1) * pauli_op::x_gate(q2) },
          { std::make_pair(q2, pauli_gate::z), pauli_op::z_gate(q1) * pauli_op::z_gate(q2) },
          { std::make_pair(q1, pauli_gate::y), pauli_op::y_gate(q1) * pauli_op::x_gate(q2) },
          { std::make_pair(q2, pauli_gate::y), pauli_op::z_gate(q1) * pauli_op::y_gate(q2) } });
    }
    static clifford_op x_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::z), -(pauli_op::z_gate(q)) },
          { std::make_pair(q, pauli_gate::y), -(pauli_op::y_gate(q)) } });
    }
    static clifford_op z_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), -(pauli_op::x_gate(q)) },
          { std::make_pair(q, pauli_gate::y), -(pauli_op::y_gate(q)) } });
    }
    static clifford_op y_gate(id q) {
      return clifford_op(
        { { std::make_pair(q, pauli_gate::x), -(pauli_op::x_gate(q)) },
          { std::make_pair(q, pauli_gate::z), -(pauli_op::z_gate(q)) } });
    }

    /* Operators */
    pauli_op conjugate(const pauli_op& P) const {
      pauli_op ret;
      ret *= P.phase();
      
      P.for_each([&ret, this](auto& p) {
          auto it = this->perm_.find(p);
          if (it == perm_.end()) {
            ret *= pauli_op(p);
          } else {
            ret *= it->second;
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

    /* Printing */
    std::ostream& print(std::ostream& os) const {
      os << "{ ";
      for (auto& [pauli_in, pauli_out] : perm_) {
        os << pauli_op(pauli_in) << " --> " << pauli_out << ", ";
      }
      os << "}";

      return os;
    }

  private:
    clifford_op(std::map<std::pair<id, pauli_gate>, pauli_op> perm) : perm_(perm) {}

    std::map<std::pair<id, pauli_gate>, pauli_op> perm_;
  };

  std::ostream& operator<<(std::ostream& os, const clifford_op& P) { return P.print(os); }


  /*! \brief Class storing an uninterpreted operation on some set of qubits */
  class uninterp_op {
  public:
    uninterp_op(std::set<id> qubits) : qubits_(qubits) {}

    template<typename Fn>
    void for_each_qubit(Fn&& fn) const {
      static_assert(std::is_invocable_r_v<void, Fn, id const&>);
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
    std::set<id> qubits_;
  };
    
  std::ostream& operator<<(std::ostream& os, const uninterp_op& P) { return P.print(os); }


  /*! \brief Class storing a rotation of some angle around a pauli
   *
   *  (1 + e^i\theta)/2 I + (1 - e^i\theta) P
   */
  class rotation_op {
  public:
    rotation_op() : theta_(td::angles::zero) {}
    rotation_op(td::angle theta, pauli_op pauli) : theta_(theta), pauli_(pauli) {}

    /* Gate constructors */
    static rotation_op t_gate(id q) { return rotation_op(td::angles::pi_quarter, pauli_op::z_gate(q)); }
    static rotation_op tdg_gate(id q) { return rotation_op(-td::angles::pi_quarter, pauli_op::z_gate(q)); }
    static rotation_op rz_gate(td::angle theta, id q) { return rotation_op(theta, pauli_op::z_gate(q)); }
    static rotation_op rx_gate(td::angle theta, id q) { return rotation_op(theta, pauli_op::x_gate(q)); }
    static rotation_op ry_gate(td::angle theta, id q) { return rotation_op(theta, pauli_op::y_gate(q)); }

    /* Operators */

    // CR(theta, P) == R(theta, P')C

    rotation_op commute_left(const clifford_op& C) const {
      auto tmp = rotation_op(*this);
      tmp.pauli_ = C.conjugate(tmp.pauli_);
      return tmp;
    }

    bool commutes_with(const rotation_op& R) const {
      return pauli_.commutes_with(R.pauli_);
    }

    bool commutes_with(const uninterp_op& U) const {
      auto tmp = true;

      U.for_each_qubit([&tmp, this](id q) {
          tmp &= pauli_.trivial_on(q);
        });

      return tmp;
    }

    std::optional<std::pair<td::angle, rotation_op> > try_merge(const rotation_op& R) const {
      if (pauli_ == R.pauli_) {
        auto phase = td::angles::zero;
        auto rotation = rotation_op(theta_ + R.theta_, pauli_);
        return std::make_optional(std::make_pair(phase, rotation));
      } else if (pauli_ == -(R.pauli_)) {
        auto phase = R.theta_;
        auto rotation = rotation_op(theta_ + -R.theta_, pauli_);
        return std::make_optional(std::make_pair(phase, rotation));
      } else {
        return std::nullopt;
      }
    }

    /* Printing */
    std::ostream& print(std::ostream& os) const {
      os << "R(" << theta_ << ", " << pauli_ << ")";

      return os;
    }

  private:
    td::angle theta_;
    pauli_op pauli_;

  };

  std::ostream& operator<<(std::ostream& os, const rotation_op& P) { return P.print(os); }

  typedef std::variant<rotation_op, clifford_op, uninterp_op> op;

}
}
