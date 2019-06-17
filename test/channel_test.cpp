#define FMT_HEADER_ONLY = true

#include "optimization/channel.hpp"
#include <unordered_map>
#include <iostream>

using namespace synthewareQ::channel;

int main() {
  auto a = pauli_op::i_gate("x1");
  auto b = pauli_op::x_gate("x1");
  auto c = pauli_op::z_gate("x1");
  auto d = pauli_op::y_gate("x1");

  auto e = c * b;
  a *= e;

  std::cout << a << "*" << b << "*" << c << "*" << d << "=" << a*b*c*d << "\n";
  std::cout << "\n";

  auto cliff1 = clifford_op::h_gate("x1");
  auto cliff2 = clifford_op::s_gate("x1");
  auto cliff3 = clifford_op::cnot_gate("x1", "x2");

  std::cout << "H: " << cliff1 << "\n"; 
  std::cout << "S: " << cliff2 << "\n";
  std::cout << "CNOT: " << cliff3 << "\n";
  std::cout << "HSH: " << cliff1 * cliff2 * cliff1 << "\n";
  std::cout << "(I H)CNOT(I H): " << clifford_op::h_gate("x2") * cliff3 * clifford_op::h_gate("x2") << "\n";
  std::cout << "\n";

  std::cout << "HXH: " << cliff1.conjugate(b) << "\n";
  std::cout << "HZH: " << cliff1.conjugate(c) << "\n";
  std::cout << "HYH: " << cliff1.conjugate(d) << "\n";
  std::cout << "CNOT(X I)CNOT" << cliff3.conjugate(b) << "\n";
  std::cout << "CNOT(I X)CNOT" << cliff3.conjugate(pauli_op::x_gate("x2")) << "\n";

  return 1;
}
