#include <bit>
#include <cassert>
#include <cmath>
#include <complex>
#include <format>
#include <iostream>
#include <numbers>
#include <print>
#include <set>
#include <tools_v1/algorithm/Observable.hpp>
#include <vector>

// ANSI color codes for terminal output
namespace colors {
constexpr auto reset = "\033[0m";
constexpr auto red = "\033[31m";
constexpr auto green = "\033[32m";
constexpr auto yellow = "\033[33m";
constexpr auto blue = "\033[34m";
constexpr auto magenta = "\033[35m";
constexpr auto cyan = "\033[36m";
constexpr auto bold = "\033[1m";
constexpr auto underline = "\033[4m";

// Combined styles
constexpr auto bold_red = "\033[1;31m";
constexpr auto bold_green = "\033[1;32m";
constexpr auto bold_yellow = "\033[1;33m";
constexpr auto bold_blue = "\033[1;34m";
constexpr auto bold_magenta = "\033[1;35m";
constexpr auto bold_cyan = "\033[1;36m";
} // namespace colors

using namespace tools_v1::algorithm;

// Test function 1: Creation and annihilation operators
void test_creation_annihilation_operators() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 1: Creation and Annihilation Operators ===" << colors::reset << std::endl;

  int lattice_size = 4;
  qbit ancilla(0);

  // Test creation operator
  std::cout << "\n" << colors::bold_yellow << "Creation operator for site 2:" << colors::reset << std::endl;
  circuit creation_circuit = create_creation_operator(2, lattice_size, ancilla);
  std::cout << colors::bold_green << "Creation operator circuit:" << colors::reset << std::endl;
  std::cout << colors::green << creation_circuit << colors::reset << std::endl;
  analyze_observable_circuit(creation_circuit, lattice_size);

  // Test annihilation operator
  std::cout << "\n" << colors::bold_yellow << "Annihilation operator for site 1:" << colors::reset << std::endl;
  circuit annihilation_circuit = create_annihilation_operator(1, lattice_size, ancilla);
  std::cout << colors::bold_green << "Annihilation operator circuit:" << colors::reset << std::endl;
  std::cout << colors::green << annihilation_circuit << colors::reset << std::endl;
  analyze_observable_circuit(annihilation_circuit, lattice_size);
}

// Test function 2: Kinetic term A
void test_kinetic_term_A() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 2: Kinetic Term A ===" << colors::reset << std::endl;

  std::vector<int> lattice_sizes = {2, 4, 6};
  std::vector<double> hopping_strengths = {0.5, 1.0, 2.0};

  for (int L : lattice_sizes) {
    for (double t : hopping_strengths) {
      std::cout << "\n" << colors::bold_yellow << "Kinetic term A for L = " << L
                << ", t = " << t << ":" << colors::reset << std::endl;

      circuit A_circuit = create_kinetic_term_A(L, t);
      std::cout << colors::bold_green << "Kinetic term A circuit:" << colors::reset << std::endl;
      std::cout << colors::green << A_circuit << colors::reset << std::endl;
      analyze_observable_circuit(A_circuit, L);
    }
  }
}

// Test function 3: First inversion (z-i-A+E)^{-1}
void test_first_inversion() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 3: First Inversion (z-i-A+E)^{-1} ===" << colors::reset << std::endl;

  int lattice_size = 4;
  std::complex<double> z(1.0, 0.5);
  double E = 0.1;
  std::vector<double> qsvt_phases = {0.1, 0.2, 0.3, 0.4, 0.5};

  std::cout << "\n" << colors::bold_yellow << "First inversion for z = " << z.real() << " + i" << z.imag()
            << ", E = " << E << ":" << colors::reset << std::endl;

  circuit first_inversion = create_first_inversion(lattice_size, z, E, qsvt_phases);
  std::cout << colors::bold_green << "First inversion circuit:" << colors::reset << std::endl;
  std::cout << colors::green << first_inversion << colors::reset << std::endl;
  analyze_observable_circuit(first_inversion, lattice_size);
}

// Test function 4: Second inversion (I + (z-i-A+E)^{-1}(i-B))^{-1}
void test_second_inversion() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 4: Second Inversion (I + (z-i-A+E)^{-1}(i-B))^{-1} ===" << colors::reset << std::endl;

  int lattice_size = 4;
  std::complex<double> z(1.0, 0.5);
  double E = 0.1;
  std::vector<double> qsvt_phases_first = {0.1, 0.2, 0.3, 0.4, 0.5};
  std::vector<double> qsvt_phases_second = {0.2, 0.3, 0.4, 0.5, 0.6};

  // Create first inversion for input
  circuit first_inversion = create_first_inversion(lattice_size, z, E, qsvt_phases_first);

  std::cout << "\n" << colors::bold_yellow << "Second inversion for z = " << z.real() << " + i" << z.imag()
            << ", E = " << E << ":" << colors::reset << std::endl;

  circuit second_inversion = create_second_inversion(lattice_size, z, E, first_inversion, qsvt_phases_second);
  std::cout << colors::bold_green << "Second inversion circuit:" << colors::reset << std::endl;
  std::cout << colors::green << second_inversion << colors::reset << std::endl;
  analyze_observable_circuit(second_inversion, lattice_size);
}

// Test function 5: Complete observable circuit
void test_complete_observable() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 5: Complete Observable Circuit ===" << colors::reset << std::endl;

  int lattice_size = 4;
  int site_i = 1;
  int site_j = 2;
  std::complex<double> z(1.0, 0.5);
  double E = 0.1;
  std::vector<double> qsvt_phases_first = {0.1, 0.2, 0.3, 0.4, 0.5};
  std::vector<double> qsvt_phases_second = {0.2, 0.3, 0.4, 0.5, 0.6};

  std::cout << "\n" << colors::bold_yellow << "Complete observable for sites " << site_i << " -> " << site_j
            << ", z = " << z.real() << " + i" << z.imag() << ", E = " << E << ":" << colors::reset << std::endl;

  circuit observable_circuit = create_observable_circuit(lattice_size, site_i, site_j, z, E,
                                                        qsvt_phases_first, qsvt_phases_second);
  std::cout << colors::bold_green << "Complete observable circuit:" << colors::reset << std::endl;
  std::cout << colors::green << observable_circuit << colors::reset << std::endl;
  analyze_observable_circuit(observable_circuit, lattice_size);
}

// Test function 6: Hadamard test for expectation values
void test_hadamard_test() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 6: Hadamard Test for Expectation Values ===" << colors::reset << std::endl;

  int lattice_size = 4;
  int site_i = 1;
  int site_j = 2;
  std::complex<double> z(1.0, 0.5);
  double E = 0.1;
  std::vector<double> qsvt_phases_first = {0.1, 0.2, 0.3, 0.4, 0.5};
  std::vector<double> qsvt_phases_second = {0.2, 0.3, 0.4, 0.5, 0.6};

  // Create observable circuit
  circuit observable_circuit = create_observable_circuit(lattice_size, site_i, site_j, z, E,
                                                        qsvt_phases_first, qsvt_phases_second);

  // Create Hadamard test circuit
  qbit test_ancilla(lattice_size * 2 + 1);
  circuit hadamard_test = create_hadamard_test(observable_circuit, test_ancilla);

  std::cout << "\n" << colors::bold_yellow << "Hadamard test circuit for observable measurement:" << colors::reset << std::endl;
  std::cout << colors::bold_green << "Hadamard test circuit:" << colors::reset << std::endl;
  std::cout << colors::green << hadamard_test << colors::reset << std::endl;
  analyze_observable_circuit(hadamard_test, lattice_size);
}

// Test function 7: Circuit scaling analysis
void test_circuit_scaling() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 7: Circuit Scaling Analysis ===" << colors::reset << std::endl;

  std::vector<int> lattice_sizes = {2, 4, 6};
  std::complex<double> z(1.0, 0.5);
  double E = 0.1;
  std::vector<double> qsvt_phases = {0.1, 0.2, 0.3, 0.4, 0.5};

  std::cout << "\n" << colors::bold_yellow << "Circuit size scaling with lattice size:" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "Lattice Size | Creation Op | Annihilation Op | Kinetic A | First Inv | Second Inv | Complete Obs" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "-------------|------------|-----------------|-----------|-----------|------------|-------------" << colors::reset << std::endl;

  for (int L : lattice_sizes) {
    qbit ancilla(0);
    circuit creation_op = create_creation_operator(1, L, ancilla);
    circuit annihilation_op = create_annihilation_operator(1, L, ancilla);
    circuit kinetic_A = create_kinetic_term_A(L);
    circuit first_inv = create_first_inversion(L, z, E, qsvt_phases);
    circuit second_inv = create_second_inversion(L, z, E, first_inv, qsvt_phases);
    circuit complete_obs = create_observable_circuit(L, 1, 2, z, E, qsvt_phases, qsvt_phases);

    std::cout << colors::cyan << "     " << L << "      |"
              << "     " << creation_op.size() << "     |"
              << "        " << annihilation_op.size() << "       |"
              << "     " << kinetic_A.size() << "    |"
              << "     " << first_inv.size() << "    |"
              << "      " << second_inv.size() << "     |"
              << "      " << complete_obs.size() << colors::reset << std::endl;
  }
}

// Test function 8: Parameter sensitivity
void test_parameter_sensitivity() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 8: Parameter Sensitivity ===" << colors::reset << std::endl;

  int lattice_size = 4;
  int site_i = 1;
  int site_j = 2;
  std::vector<double> qsvt_phases = {0.1, 0.2, 0.3, 0.4, 0.5};

  std::vector<std::pair<std::complex<double>, double>> parameters = {
    {{1.0, 0.0}, 0.1},  // z = 1+0i, E = 0.1
    {{1.0, 0.5}, 0.1},  // z = 1+0.5i, E = 0.1
    {{1.0, 1.0}, 0.1},  // z = 1+1i, E = 0.1
    {{1.0, 0.5}, 0.5},  // z = 1+0.5i, E = 0.5
    {{1.0, 0.5}, 1.0},  // z = 1+0.5i, E = 1.0
  };

  std::cout << "\n" << colors::bold_yellow << "Circuit size vs parameters (L = " << lattice_size << "):" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "z (real) | z (imag) | E | Complete Observable Gates" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "---------|----------|---|-------------------------" << colors::reset << std::endl;

  for (const auto &[z, E] : parameters) {
    circuit complete_obs = create_observable_circuit(lattice_size, site_i, site_j, z, E,
                                                    qsvt_phases, qsvt_phases);

    std::cout << colors::cyan << "   " << z.real() << "   |"
              << "    " << z.imag() << "    |"
              << " " << E << " |"
              << "           " << complete_obs.size() << colors::reset << std::endl;
  }
}

// Test function 9: Circuit structure verification
void test_circuit_structure() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 9: Circuit Structure Verification ===" << colors::reset << std::endl;

  int lattice_size = 3;
  int site_i = 1;
  int site_j = 2;
  std::complex<double> z(1.0, 0.5);
  double E = 0.1;
  std::vector<double> qsvt_phases = {0.1, 0.2, 0.3, 0.4, 0.5};

  std::cout << "\n" << colors::bold_yellow << "Detailed circuit structure for L = " << lattice_size << ":" << colors::reset << std::endl;

  // Test creation operator
  std::cout << "\n" << colors::bold_green << "Creation Operator Gates:" << colors::reset << std::endl;
  qbit ancilla(0);
  circuit creation_circuit = create_creation_operator(site_j, lattice_size, ancilla);
  int gate_count = 0;
  for (const auto &gate : creation_circuit) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate << std::endl;
    }
  }

  // Test annihilation operator
  std::cout << "\n" << colors::bold_blue << "Annihilation Operator Gates:" << colors::reset << std::endl;
  circuit annihilation_circuit = create_annihilation_operator(site_i, lattice_size, ancilla);
  gate_count = 0;
  for (const auto &gate : annihilation_circuit) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate << std::endl;
    }
  }

  // Test complete observable
  std::cout << "\n" << colors::bold_magenta << "Complete Observable Circuit Gates:" << colors::reset << std::endl;
  circuit observable_circuit = create_observable_circuit(lattice_size, site_i, site_j, z, E,
                                                        qsvt_phases, qsvt_phases);
  gate_count = 0;
  for (const auto &gate : observable_circuit) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate << std::endl;
    }
  }
}

int main() {
  std::cout << colors::bold_cyan << "Running Observable.cpp Tests" << colors::reset << std::endl;
  std::cout << colors::bold_cyan << "============================" << colors::reset << std::endl;

  // Run all tests
  test_creation_annihilation_operators();
  test_kinetic_term_A();
  test_first_inversion();
  test_second_inversion();
  test_complete_observable();
  test_hadamard_test();
  test_circuit_scaling();
  test_parameter_sensitivity();
  test_circuit_structure();

  std::cout << "\n" << colors::bold_green << "All Observable tests completed successfully!" << colors::reset << std::endl;

  return 0;
}