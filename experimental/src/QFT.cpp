#include <bit>
#include <cassert>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <print>
#include <set>
#include <tools_v1/algorithm/QFT.hpp>
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

// Test function 1: Basic QFT functionality
void test_basic_qft() {
  std::println("\n{}=== TEST 1: Basic QFT Functionality ==={}",
               colors::bold_cyan, colors::reset);

  // Test with 3 qubits
  std::println("\n{}QFT on 3 qubits:{}", colors::bold_yellow, colors::reset);
  std::vector<qbit> qubits3 = {qbit(0), qbit(1), qbit(2)};
  circuit qft3 = QFT(qubits3);

  // Debug: Print each gate individually
  std::println("{}Debug - Individual gates in QFT circuit:{}", colors::bold_red,
               colors::reset);
  int gate_count = 0;
  for (const auto &gate : qft3) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate;
    }
  }

  std::println("\n{}Full circuit output:{}", colors::bold_green, colors::reset);
  std::cout << colors::green << qft3 << colors::reset << std::endl;

  // Test with 4 qubits
  std::println("\n{}QFT on 4 qubits:{}", colors::bold_yellow, colors::reset);
  std::vector<qbit> qubits4 = {qbit(0), qbit(1), qbit(2), qbit(3)};
  circuit qft4 = QFT(qubits4);
  std::cout << colors::blue << qft4 << colors::reset << std::endl;
}

// Test function 2: Inverse QFT
void test_inverse_qft() {
  std::println("\n{}=== TEST 2: Inverse QFT ==={}", colors::bold_cyan,
               colors::reset);

  // Test with 3 qubits
  std::println("\n{}Inverse QFT on 3 qubits:{}", colors::bold_yellow,
               colors::reset);
  std::vector<qbit> qubits3 = {qbit(0), qbit(1), qbit(2)};
  circuit inv_qft3 = inverse_QFT(qubits3);
  std::cout << colors::green << inv_qft3 << colors::reset << std::endl;
}

// Test function 3: QFT followed by inverse QFT (should be identity)
void test_qft_roundtrip() {
  std::println("\n{}=== TEST 3: QFT Roundtrip ==={}", colors::bold_cyan,
               colors::reset);

  std::println("\n{}QFT followed by inverse QFT on 2 qubits:{}",
               colors::bold_yellow, colors::reset);
  std::vector<qbit> qubits2 = {qbit(0), qbit(1)};

  circuit qft2 = QFT(qubits2);
  circuit inv_qft2 = inverse_QFT(qubits2);

  std::println("{}Forward QFT:{}", colors::bold_green, colors::reset);
  std::cout << colors::green << qft2 << colors::reset << std::endl;

  std::println("{}Inverse QFT:{}", colors::bold_blue, colors::reset);
  std::cout << colors::blue << inv_qft2 << colors::reset << std::endl;

  std::println("{}Note: QFT followed by inverse QFT should be identity{}",
               colors::bold_magenta, colors::reset);
}

// Test function 4: Phase rotation gates
void test_phase_rotations() {
  std::println("\n{}=== TEST 4: Phase Rotation Gates ==={}", colors::bold_cyan,
               colors::reset);

  std::println("\n{}Individual phase rotation gates:{}", colors::bold_yellow,
               colors::reset);

  // Test R2 gate
  std::println("\n{}R2 gate (π/2 rotation):{}", colors::bold_green,
               colors::reset);
  auto r2 = phase_rotation(2, qbit(0));
  if (r2) {
    std::cout << colors::green << *r2 << colors::reset << std::endl;
  } else {
    std::println("{}Failed to create R2 gate{}", colors::bold_red,
                 colors::reset);
  }

  // Test R3 gate
  std::println("\n{}R3 gate (π/4 rotation):{}", colors::bold_blue,
               colors::reset);
  auto r3 = phase_rotation(3, qbit(0));
  if (r3) {
    std::cout << colors::blue << *r3 << colors::reset << std::endl;
  } else {
    std::println("{}Failed to create R3 gate{}", colors::bold_red,
                 colors::reset);
  }

  // Test controlled R2 gate
  std::println("\n{}Controlled R2 gate:{}", colors::bold_magenta,
               colors::reset);
  auto controlled_r2 = controlled_phase_rotation(2, qbit(1), qbit(0));
  if (controlled_r2) {
    std::cout << colors::magenta << *controlled_r2 << colors::reset
              << std::endl;
  } else {
    std::println("{}Failed to create controlled R2 gate{}", colors::bold_red,
                 colors::reset);
  }

  // Test a small circuit with phase gates
  std::println("\n{}Small circuit with phase gates:{}", colors::bold_yellow,
               colors::reset);
  circuit phase_circuit;
  phase_circuit.push_back(phase_rotation(2, qbit(0)));
  phase_circuit.push_back(phase_rotation(3, qbit(1)));
  phase_circuit.push_back(controlled_phase_rotation(2, qbit(1), qbit(0)));
  std::cout << colors::cyan << phase_circuit << colors::reset << std::endl;
}

// Test function 5: QFT circuit analysis
void test_qft_analysis() {
  std::println("\n{}=== TEST 5: QFT Circuit Analysis ==={}", colors::bold_cyan,
               colors::reset);

  std::vector<int> qubit_counts = {2, 3, 4, 5};

  for (int n : qubit_counts) {
    std::println("\n{}QFT with {} qubits:{}", colors::bold_yellow, n,
                 colors::reset);

    std::vector<qbit> qubits;
    for (int i = 0; i < n; ++i) {
      qubits.push_back(qbit(i));
    }

    circuit qft_circuit = QFT(qubits);

    std::println("{}Number of gates: {}{}", colors::green, qft_circuit.size(),
                 colors::reset);

    // Calculate expected number of gates
    int expected_hadamards = n;
    int expected_controlled_rotations = n * (n - 1) / 2;
    int expected_total = expected_hadamards + expected_controlled_rotations;

    std::println(
        "{}Expected gates: {} ({} Hadamards + {} controlled rotations){}",
        colors::blue, expected_total, expected_hadamards,
        expected_controlled_rotations, colors::reset);

    if (qft_circuit.size() == expected_total) {
      std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                   colors::reset);
    } else {
      std::println("{}✗ Gate count mismatch{}", colors::bold_red,
                   colors::reset);
    }

    // Show first few gates for larger circuits
    if (n > 3) {
      std::println("{}First 5 gates:{}", colors::magenta, colors::reset);
      int count = 0;
      for (const auto &gate : qft_circuit) {
        if (count >= 5)
          break;
        if (gate) {
          std::cout << "  " << *gate << std::endl;
          count++;
        }
      }
      if (qft_circuit.size() > 5) {
        std::println("{}... and {} more gates{}", colors::cyan,
                     qft_circuit.size() - 5, colors::reset);
      }
    } else {
      std::cout << colors::green << qft_circuit << colors::reset << std::endl;
    }
  }
}

int main() {
  std::println("{}Running QFT.cpp Tests{}", colors::bold_cyan, colors::reset);
  std::println("{}===================={}", colors::bold_cyan, colors::reset);

  // Run all tests
  test_basic_qft();
  test_inverse_qft();
  test_qft_roundtrip();
  test_phase_rotations();
  test_qft_analysis();

  return 0;
}
