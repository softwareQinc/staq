#include <cassert>
#include <cmath>
#include <iostream>
#include <numbers>
#include <tools_v1/algorithm/QSVT.hpp>
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

// Test function 1: Basic QSVT functionality
void test_basic_qsvt() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 1: Basic QSVT Functionality ===" << colors::reset
            << std::endl;

  // Test parameters for d=1 polynomial (3 phases)
  std::vector<double> phi = {0.1, 0.2, 0.3};

  std::cout << "\n"
            << colors::bold_yellow
            << "QSVT with 3 phases (d=1):" << colors::reset << std::endl;

  // Create test qubits
  std::vector<qbit> control_ancilla = {qbit(1), qbit(2)}; // 2 control ancillas
  qbit qsvt_ancilla(0);                           // QSVT ancilla

  // Create a simple test unitary (Hadamard on data qubit)
  circuit test_u;
  qbit data_qubit(3);
  test_u.push_back(hadamard(data_qubit));
  for (const auto &ctrl : control_ancilla) {
    test_u.save_ancilla(ctrl);
  }

  std::cout << "Test parameters:" << std::endl;
  std::cout << "- Number of phases: " << phi.size() << std::endl;
  std::cout << "- Control ancillas: " << control_ancilla.size() << std::endl;

  try {
    circuit qsvt_result = QSVT(phi, test_u, qsvt_ancilla);

    std::cout << "\n"
              << colors::bold_green << "✅ QSVT function executed successfully!"
              << colors::reset << std::endl;
    std::cout << "Circuit size: " << qsvt_result.size() << " gates"
              << std::endl;

    std::cout << "\n"
              << colors::bold_green << "Full QSVT circuit:" << colors::reset
              << std::endl;
    std::cout << colors::green << qsvt_result << colors::reset << std::endl;

  } catch (const std::exception &e) {
    std::cout << "\n"
              << colors::bold_red << "❌ Error in QSVT function: " << e.what()
              << colors::reset << std::endl;
  }
}

// Test function 2: QSVT with different phase counts
void test_qsvt_phase_counts() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 2: QSVT with Different Phase Counts ==="
            << colors::reset << std::endl;

  std::vector<int> phase_counts = {3, 5, 7}; // d=1, d=2, d=3

  for (int num_phases : phase_counts) {
    std::cout << "\n"
              << colors::bold_yellow << "QSVT with " << num_phases
              << " phases:" << colors::reset << std::endl;

    // Create phases
    std::vector<double> phi;
    for (int i = 0; i < num_phases; ++i) {
      phi.push_back(0.1 * (i + 1));
    }

    // Create test qubits
    std::vector<qbit> control_ancilla = {qbit(1)}; // 1 control ancilla
    qbit qsvt_ancilla(0);

    // Create test unitary
    circuit test_u;
    qbit data_qubit(2);
    test_u.push_back(hadamard(data_qubit));
    for (const auto &ctrl : control_ancilla) {
      test_u.save_ancilla(ctrl);
    }

    try {
      circuit qsvt_result = QSVT(phi, test_u, qsvt_ancilla);

      std::cout << "Circuit size: " << qsvt_result.size() << " gates"
                << std::endl;

      // Show first few gates for larger circuits
      if (num_phases > 3) {
        std::cout << colors::magenta << "First 5 gates:" << colors::reset
                  << std::endl;
        int count = 0;
        for (const auto &gate : qsvt_result) {
          if (count >= 5)
            break;
          if (gate) {
            std::cout << "  " << *gate << std::endl;
            count++;
          }
        }
        if (qsvt_result.size() > 5) {
          std::cout << colors::cyan << "... and " << qsvt_result.size() - 5
                    << " more gates" << colors::reset << std::endl;
        }
      }

    } catch (const std::exception &e) {
      std::cout << colors::bold_red << "❌ Error: " << e.what() << colors::reset
                << std::endl;
    }
  }
}

// Test function 3: QSVT circuit analysis
void test_qsvt_analysis() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 3: QSVT Circuit Analysis ===" << colors::reset
            << std::endl;

  // Test with different numbers of ancilla qubits
  std::vector<int> ancilla_counts = {1, 2, 3};

  for (int num_ancilla : ancilla_counts) {
    std::cout << "\n"
              << colors::bold_yellow << "QSVT with " << num_ancilla
              << " ancilla qubits:" << colors::reset << std::endl;

    // Create phases for d=1 polynomial
    std::vector<double> phi = {0.1, 0.2, 0.3};

    // Create ancilla qubits
    std::vector<qbit> control_ancilla;
    for (int i = 0; i < num_ancilla; ++i) {
      control_ancilla.push_back(qbit(i + 1));
    }
    qbit qsvt_ancilla(0);

    // Create test unitary
    circuit test_u;
    qbit data_qubit(num_ancilla + 1);
    test_u.push_back(hadamard(data_qubit));
    for (const auto &ctrl : control_ancilla) {
      test_u.save_ancilla(ctrl);
    }

    try {
      circuit qsvt_result = QSVT(phi, test_u, qsvt_ancilla);

      std::cout << colors::green << "Number of gates: " << qsvt_result.size()
                << colors::reset << std::endl;

      // Calculate expected gate counts
      int expected_cnots = 2 * num_ancilla * phi.size(); // CNOTs for each phase
      int expected_rotations = phi.size();     // One rotation per phase
      int expected_hadamards = 2;              // Initial and final
      int expected_unitaries = phi.size() - 1; // U/U† applications
      int expected_total = expected_cnots + expected_rotations +
                           expected_hadamards + expected_unitaries;

      std::cout << colors::blue << "Expected gates: " << expected_total << " ("
                << expected_cnots << " CNOTs + " << expected_rotations
                << " rotations + " << expected_hadamards << " Hadamards + "
                << expected_unitaries << " unitaries)" << colors::reset
                << std::endl;

      if (qsvt_result.size() == expected_total) {
        std::cout << colors::bold_green << "✓ Gate count matches expected"
                  << colors::reset << std::endl;
      } else {
        std::cout << colors::bold_red << "✗ Gate count mismatch"
                  << colors::reset << std::endl;
      }

    } catch (const std::exception &e) {
      std::cout << colors::bold_red << "❌ Error: " << e.what() << colors::reset
                << std::endl;
    }
  }
}

// Test function 4: Alternative QSVT implementation
void test_alternative_qsvt() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 4: Alternative QSVT Implementation ==="
            << colors::reset << std::endl;

  std::vector<double> phi = {0.1, 0.2, 0.3, 0.4, 0.5}; // 5 phases

  std::cout << "\n"
            << colors::bold_yellow
            << "Alternative QSVT with 5 phases:" << colors::reset << std::endl;

  std::vector<qbit> control_ancilla = {qbit(1)};
  qbit qsvt_ancilla(0);

  circuit test_u;
  qbit data_qubit(2);
  test_u.push_back(hadamard(data_qubit));
  for (const auto &ctrl : control_ancilla) {
    test_u.save_ancilla(ctrl);
  }

  try {
    std::cout << colors::yellow
              << "Alternative implementation unavailable; running baseline QSVT"
              << colors::reset << std::endl;
    circuit qsvt_result = QSVT(phi, test_u, qsvt_ancilla);

    std::cout << colors::green
              << "Alternative QSVT circuit size: " << qsvt_result.size()
              << " gates" << colors::reset << std::endl;

    std::cout << colors::blue << "First 8 gates:" << colors::reset << std::endl;
    int count = 0;
    for (const auto &gate : qsvt_result) {
      if (count >= 8)
        break;
      if (gate) {
        std::cout << "  " << *gate << std::endl;
        count++;
      }
    }
    if (qsvt_result.size() > 8) {
      std::cout << colors::cyan << "... and " << qsvt_result.size() - 8
                << " more gates" << colors::reset << std::endl;
    }

  } catch (const std::exception &e) {
    std::cout << colors::bold_red
              << "❌ Error in alternative QSVT: " << e.what() << colors::reset
              << std::endl;
  }
}

// Test function 5: Z rotation gates
void test_z_rotations() {
  std::cout << "\n"
            << colors::bold_cyan
            << "=== TEST 5: Z Rotation Gates ===" << colors::reset << std::endl;

  std::cout << "\n"
            << colors::bold_yellow
            << "Individual Z rotation gates:" << colors::reset << std::endl;

  // Test Rz(π/4) gate
  std::cout << "\n"
            << colors::bold_green << "Rz(π/4) gate:" << colors::reset
            << std::endl;
  auto rz_pi4 = rz_gate(std::numbers::pi / 4.0, qbit(0));
  if (rz_pi4) {
    std::cout << colors::green << *rz_pi4 << colors::reset << std::endl;
  } else {
    std::cout << colors::bold_red << "Failed to create Rz(π/4) gate"
              << colors::reset << std::endl;
  }

  // Test controlled Rz(π/2) gate
  std::cout << "\n"
            << colors::bold_blue << "Controlled Rz(π/2) gate:" << colors::reset
            << std::endl;
  auto controlled_rz =
      controlled_rz_gate(std::numbers::pi / 2.0, qbit(1), qbit(0));
  if (controlled_rz) {
    std::cout << colors::blue << *controlled_rz << colors::reset << std::endl;
  } else {
    std::cout << colors::bold_red << "Failed to create controlled Rz(π/2) gate"
              << colors::reset << std::endl;
  }

  // Test multi-controlled Rz gate
  std::cout << "\n"
            << colors::bold_magenta
            << "Multi-controlled Rz(π/3) gate:" << colors::reset << std::endl;
  std::vector<qbit> controls = {qbit(1), qbit(2)};
  auto multi_controlled_rz =
      multi_controlled_rz_gate(std::numbers::pi / 3.0, controls, qbit(0));
  if (multi_controlled_rz) {
    std::cout << colors::magenta << *multi_controlled_rz << colors::reset
              << std::endl;
  } else {
    std::cout << colors::bold_red << "Failed to create multi-controlled Rz gate"
              << colors::reset << std::endl;
  }
}

int main() {
  std::cout << colors::bold_cyan << "Running QSVT.cpp Tests" << colors::reset
            << std::endl;
  std::cout << colors::bold_cyan << "=====================" << colors::reset
            << std::endl;

  // Run all tests
  test_basic_qsvt();
  test_qsvt_phase_counts();
  test_qsvt_analysis();
  test_alternative_qsvt();
  test_z_rotations();

  return 0;
}
