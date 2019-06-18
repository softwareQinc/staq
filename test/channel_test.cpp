#define FMT_HEADER_ONLY = true

#include "optimization/channel.hpp"
#include <unordered_map>
#include <set>
#include <iostream>

using namespace synthewareQ::channel;

void print_merge(rotation_op R1, rotation_op R2) {
  auto merged = R1.try_merge(R2);
  if (merged.has_value()) {
    auto& [phase, R] = merged.value();
    std::cout << "e^i(" << phase << ")" << R;
  } else {
    std::cout << R1 << R2;
  }
}


int main() {
  pauli_op empty;
  auto a = pauli_op::i_gate("x1");
  auto b = pauli_op::x_gate("x1");
  auto c = pauli_op::z_gate("x1");
  auto d = pauli_op::y_gate("x1");

  std::cout << a << "*" << b << "*" << c << "*" << d << "=" << a*b*c*d << "\n";
  std::cout << "\n";

  std::cout << "X = I? " << (b == empty ? "true" : "false") << "\n";
  std::cout << "XX = I? " << ((b * b) == empty ? "true" : "false") << "\n";
  std::cout << "XZY = I? " << ((b * c * d) == empty ? "true" : "false") << "\n";

  std::cout << "\n";

  auto x1 = pauli_op::x_gate("x1");
  auto x2 = pauli_op::x_gate("x2");
  auto z1 = pauli_op::z_gate("x1");
  auto z2 = pauli_op::z_gate("x2");
  std::cout << "[" << x1 << ", " << x1 << "] = " << (x1.commutes_with(x1) ? "1" : "-1") << "\n";
  std::cout << "[" << x1 << ", " << z1 << "] = " << (x1.commutes_with(z1) ? "1" : "-1") << "\n";
  std::cout << "[" << x1 << ", " << z2 << "] = " << (x1.commutes_with(z2) ? "1" : "-1") << "\n";
  std::cout << "[" << x1*z2 << ", " << z1*x2 << "] = " << ((x1*z2).commutes_with(z1*x2) ? "1" : "-1") << "\n";
  std::cout << "\n";

  auto h1 = clifford_op::h_gate("x1");
  auto s1 = clifford_op::s_gate("x1");
  auto cnot12 = clifford_op::cnot_gate("x1", "x2");

  std::cout << "H: " << h1 << "\n"; 
  std::cout << "HH: " << h1 * h1 << "\n"; 
  std::cout << "S: " << s1 << "\n";
  std::cout << "SS: " << s1 * s1 << "\n";
  std::cout << "SS*: " << s1 * clifford_op::sdg_gate("x1") << "\n";
  std::cout << "CNOT: " << cnot12 << "\n";
  std::cout << "CNOTCNOT: " << cnot12 * cnot12 << "\n";
  std::cout << "HSH: " << h1 * s1 * h1 << "\n";
  std::cout << "(I H)CNOT(I H): " << clifford_op::h_gate("x2") * cnot12 * clifford_op::h_gate("x2") << "\n";
  std::cout << "\n";

  std::cout << "H X(x1) H = " << h1.conjugate(b) << "\n";
  std::cout << "H Z(x1) H = " << h1.conjugate(c) << "\n";
  std::cout << "H Y(x1) H = " << h1.conjugate(d) << "\n";
  std::cout << "CNOT X(x1) CNOT = " << cnot12.conjugate(b) << "\n";
  std::cout << "CNOT X(x2) CNOT = " << cnot12.conjugate(pauli_op::x_gate("x2")) << "\n";
  std::cout << "\n";

  auto t1 = rotation_op::t_gate("x1");
  auto tdg1 = rotation_op::tdg_gate("x1");
  auto t2 = rotation_op::t_gate("x2");
  auto u1 = uninterp_op({ "x1" });

  std::cout << h1 << t1 << " = " << t1.commute_left(h1) << h1 << "\n";
  std::cout << t1 << t1 << " = "; print_merge(t1, t1); std::cout << "\n";
  std::cout << t1 << tdg1 << " = "; print_merge(t1, tdg1); std::cout << "\n";
  std::cout << t1 << t2 << " = "; print_merge(t1, t2); std::cout << "\n";
  std::cout << "\n";

  std::cout << "[" << t1 << ", " << tdg1 << "] = 1? " << (t1.commutes_with(tdg1) ? "yes" : "no") << "\n";
  std::cout << "[" << t1 << ", " << u1 << "] = 1? " << (t1.commutes_with(u1) ? "yes" : "no") << "\n";
  std::cout << "[" << t2 << ", " << u1 << "] = 1? " << (t2.commutes_with(u1) ? "yes" : "no") << "\n";

  return 1;
}
