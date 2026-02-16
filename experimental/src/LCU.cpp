#include <bit>
#include <cassert>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <print>
#include <set>
#include <tools_v1/algorithm/LCU.hpp>
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

// Test function 1: Basic LCU functionality
void test_basic_lcu() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 1: Basic LCU Functionality ===" << colors::reset << std::endl;

  // Test with 2 ancillas and 4 coefficients
  std::cout << "\n" << colors::bold_yellow << "LCU with 2 ancillas (4 coefficients):" << colors::reset << std::endl;

  std::vector<qbit> ancillas = {qbit(0), qbit(1)};
  std::vector<double> coefficients = {0.25, 0.25, 0.25, 0.25}; // Uniform

  // Create simple unitaries (identity gates for testing)
  std::vector<circuit> unitaries;
  for (int i = 0; i < 4; ++i) {
    circuit unitary;
    // Add some simple gates for testing
    unitary.push_back(hadamard(qbit(2)));
    unitaries.push_back(std::move(unitary));
  }

  circuit lcu_circuit = LCU(coefficients, ancillas, unitaries);

  std::cout << colors::bold_green << "Full LCU circuit:" << colors::reset
            << std::endl;
  std::cout << colors::green << lcu_circuit << colors::reset << std::endl;
}

// Test function 2: Two-unitaries LCU (as described in paper)
void test_two_unitaries_lcu() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 2: Two-Unitaries LCU ===" << colors::reset
            << std::endl;

  std::cout << "\n"
            << colors::bold_yellow
            << "LCU for two unitaries (paper example):" << colors::reset
            << std::endl;

  qbit ancilla(0);
  // Test coefficients
  double c0 = 1.0; // Identity
  double c1 = 0.5; // Some other coefficient

  // Create simple unitaries
  circuit U0; // Identity-like
  U0.push_back(hadamard(qbit(1)));

  circuit U1; // Another simple unitary
  U1.push_back(hadamard(qbit(2)));

  circuit lcu_circuit = LCU_two_unitaries(c0, c1, U0, U1, ancilla);

  std::cout << colors::bold_green
            << "Two-unitaries LCU circuit:" << colors::reset << std::endl;
  std::cout << colors::green << lcu_circuit << colors::reset << std::endl;
}

// Test function 3: LCU Prepare circuit
void test_lcu_prepare() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 3: LCU Prepare Circuit ===" << colors::reset
            << std::endl;

  // Test with different coefficient patterns
  std::vector<std::vector<double>> test_coefficients = {
      {0.25, 0.25, 0.25, 0.25}, // Uniform
      {0.5, 0.3, 0.15, 0.05},   // Decreasing
      {0.1, 0.4, 0.4, 0.1}      // Symmetric
  };

  for (size_t i = 0; i < test_coefficients.size(); ++i) {
    std::cout << "\n"
              << colors::bold_yellow << "Prepare circuit for coefficients "
              << (i + 1) << ":" << colors::reset << std::endl;

    std::vector<qbit> ancillas = {qbit(0), qbit(1)};
    circuit prep_circuit = lcu_prepare(test_coefficients[i], ancillas);

    std::cout << colors::blue << prep_circuit << colors::reset << std::endl;
  }
}

// Test function 4: LCU Select circuit
void test_lcu_select() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 4: LCU Select Circuit ===" << colors::reset
            << std::endl;

  std::cout << "\n"
            << colors::bold_yellow
            << "Select circuit for 2 ancillas:" << colors::reset << std::endl;

  std::vector<qbit> ancillas = {qbit(0), qbit(1)};
  std::vector<circuit> unitaries;

  // Create different unitaries for testing
  for (int i = 0; i < 4; ++i) {
    circuit unitary;
    // Each unitary applies different gates
    unitary.push_back(hadamard(qbit(2)));
    if (i % 2 == 0) {
      unitary.push_back(hadamard(qbit(3)));
    }
    unitaries.push_back(std::move(unitary));
  }

  circuit select_circuit = lcu_select(ancillas, unitaries);

  std::cout << colors::magenta << select_circuit << colors::reset << std::endl;
}

// Test function 5: LCU circuit analysis
void test_lcu_analysis() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 5: LCU Circuit Analysis ===" << colors::reset
            << std::endl;

  std::vector<int> ancilla_counts = {1, 2, 3};

  for (int n : ancilla_counts) {
    std::cout << "\n"
              << colors::bold_yellow << "LCU with " << n
              << " ancilla qubits:" << colors::reset << std::endl;

    std::vector<qbit> ancillas;
    for (int i = 0; i < n; ++i) {
      ancillas.push_back(qbit(i));
    }

    int num_coeffs = 1 << n;
    std::vector<double> coefficients(num_coeffs, 1.0 / num_coeffs); // Uniform

    std::vector<circuit> unitaries;
    for (int i = 0; i < num_coeffs; ++i) {
      circuit unitary;
      unitary.push_back(hadamard(qbit(n))); // Simple test gate
      unitaries.push_back(std::move(unitary));
    }

    circuit lcu_circuit = LCU(coefficients, ancillas, unitaries);

    std::cout << colors::green << "Number of gates: " << lcu_circuit.size()
              << colors::reset << std::endl;

    // Show first few gates for larger circuits
    if (n > 1) {
      std::cout << colors::magenta << "First 5 gates:" << colors::reset
                << std::endl;
      int count = 0;
      for (const auto &gate : lcu_circuit) {
        if (count >= 5)
          break;
        if (gate) {
          std::cout << "  " << *gate << std::endl;
          count++;
        }
      }
      if (lcu_circuit.size() > 5) {
        std::cout << colors::cyan << "... and " << (lcu_circuit.size() - 5)
                  << " more gates" << colors::reset << std::endl;
      }
    } else {
      std::cout << colors::green << lcu_circuit << colors::reset << std::endl;
    }
  }
}

int main() {
  std::cout << colors::bold_cyan << "Running LCU.cpp Tests" << colors::reset
            << std::endl;
  std::cout << colors::bold_cyan << "====================" << colors::reset
            << std::endl;

  // Run all tests
  test_basic_lcu();
  test_two_unitaries_lcu();
  test_lcu_prepare();
  test_lcu_select();
  test_lcu_analysis();

  return 0;
}
