#include <bit>
#include <cassert>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <print>
#include <set>
#include <tools_v1/algorithm/Interaction.hpp>
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

// Test function 1: Basic B term generation
void test_basic_B_term() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 1: Basic B Term Generation ===" << colors::reset << std::endl;

  // Test with different lattice sizes
  std::vector<int> lattice_sizes = {2, 4, 8};

  for (int L : lattice_sizes) {
    std::cout << "\n" << colors::bold_yellow << "B term for L = " << L << " lattice:" << colors::reset << std::endl;

    circuit B_circuit = generate_B_term(L);

    std::cout << colors::bold_green << "B term circuit:" << colors::reset << std::endl;
    std::cout << colors::green << B_circuit << colors::reset << std::endl;

    // Analyze the circuit
    analyze_interaction_circuit(B_circuit, L);
  }
}

// Test function 2: i-B term generation
void test_iB_term() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 2: i-B Term Generation ===" << colors::reset << std::endl;

  // Test with different lattice sizes
  std::vector<int> lattice_sizes = {2, 4, 6};

  for (int L : lattice_sizes) {
    std::cout << "\n" << colors::bold_yellow << "i-B term for L = " << L << " lattice:" << colors::reset << std::endl;

    circuit iB_circuit = generate_iB_term(L);

    std::cout << colors::bold_green << "i-B term circuit:" << colors::reset << std::endl;
    std::cout << colors::green << iB_circuit << colors::reset << std::endl;

    // Analyze the circuit
    analyze_interaction_circuit(iB_circuit, L);
  }
}

// Test function 3: Interaction block encoding
void test_interaction_block_encoding() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 3: Interaction Block Encoding ===" << colors::reset << std::endl;

  std::vector<int> lattice_sizes = {2, 3, 4};
  std::vector<double> interaction_strengths = {0.5, 1.0, 2.0};

  for (int L : lattice_sizes) {
    for (double U : interaction_strengths) {
      std::cout << "\n" << colors::bold_yellow << "Block encoding for L = " << L
                << ", U = " << U << ":" << colors::reset << std::endl;

      circuit block_encoding = generate_interaction_block_encoding(L, U);

      std::cout << colors::bold_green << "Block encoding circuit:" << colors::reset << std::endl;
      std::cout << colors::green << block_encoding << colors::reset << std::endl;

      // Analyze the circuit
      analyze_interaction_circuit(block_encoding, L);
    }
  }
}

// Test function 4: Complete Hubbard interaction
void test_hubbard_interaction() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 4: Complete Hubbard Interaction ===" << colors::reset << std::endl;

  // Test different parameter combinations
  std::vector<std::pair<double, double>> parameters = {
    {1.0, 0.5},  // t=1.0, U=0.5
    {1.0, 1.0},  // t=1.0, U=1.0
    {1.0, 2.0},  // t=1.0, U=2.0
    {0.5, 1.0},  // t=0.5, U=1.0
  };

  int lattice_size = 4;

  for (const auto &[t, U] : parameters) {
    std::cout << "\n" << colors::bold_yellow << "Hubbard interaction for t = "
              << t << ", U = " << U << ":" << colors::reset << std::endl;

    circuit hubbard_circuit = generate_hubbard_interaction(lattice_size, t, U);

    std::cout << colors::bold_green << "Hubbard interaction circuit:" << colors::reset << std::endl;
    std::cout << colors::green << hubbard_circuit << colors::reset << std::endl;

    // Analyze the circuit
    analyze_interaction_circuit(hubbard_circuit, lattice_size);
  }
}

// Test function 5: Circuit scaling analysis
void test_circuit_scaling() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 5: Circuit Scaling Analysis ===" << colors::reset << std::endl;

  std::vector<int> lattice_sizes = {2, 4, 6, 8};

  std::cout << "\n" << colors::bold_yellow << "Circuit size scaling with lattice size:" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "Lattice Size | B Term Gates | i-B Term Gates | Block Encoding Gates" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "-------------|--------------|----------------|-------------------" << colors::reset << std::endl;

  for (int L : lattice_sizes) {
    circuit B_circuit = generate_B_term(L);
    circuit iB_circuit = generate_iB_term(L);
    circuit block_encoding = generate_interaction_block_encoding(L);

    std::cout << colors::cyan << "     " << L << "      |"
              << "      " << B_circuit.size() << "      |"
              << "        " << iB_circuit.size() << "       |"
              << "          " << block_encoding.size() << colors::reset << std::endl;
  }
}

// Test function 6: Gate type analysis
void test_gate_type_analysis() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 6: Gate Type Analysis ===" << colors::reset << std::endl;

  int lattice_size = 4;
  double interaction_strength = 1.0;

  std::cout << "\n" << colors::bold_yellow << "Gate type distribution for L = "
            << lattice_size << ", U = " << interaction_strength << ":" << colors::reset << std::endl;

  // Test different circuit types
  std::cout << "\n" << colors::bold_green << "B Term:" << colors::reset << std::endl;
  analyze_interaction_circuit(generate_B_term(lattice_size, interaction_strength), lattice_size);

  std::cout << "\n" << colors::bold_green << "i-B Term:" << colors::reset << std::endl;
  analyze_interaction_circuit(generate_iB_term(lattice_size, interaction_strength), lattice_size);

  std::cout << "\n" << colors::bold_green << "Block Encoding:" << colors::reset << std::endl;
  analyze_interaction_circuit(generate_interaction_block_encoding(lattice_size, interaction_strength), lattice_size);

  std::cout << "\n" << colors::bold_green << "Hubbard Interaction:" << colors::reset << std::endl;
  analyze_interaction_circuit(generate_hubbard_interaction(lattice_size, 1.0, interaction_strength), lattice_size);
}

// Test function 7: Parameter sensitivity
void test_parameter_sensitivity() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 7: Parameter Sensitivity ===" << colors::reset << std::endl;

  int lattice_size = 4;
  std::vector<double> interaction_strengths = {0.1, 0.5, 1.0, 2.0, 5.0};

  std::cout << "\n" << colors::bold_yellow << "Circuit size vs interaction strength (L = "
            << lattice_size << "):" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "Interaction Strength | B Term Gates | i-B Term Gates" << colors::reset << std::endl;
  std::cout << colors::bold_magenta << "-------------------|--------------|----------------" << colors::reset << std::endl;

  for (double U : interaction_strengths) {
    circuit B_circuit = generate_B_term(lattice_size, U);
    circuit iB_circuit = generate_iB_term(lattice_size, U);

    std::cout << colors::cyan << "         " << U << "         |"
              << "      " << B_circuit.size() << "      |"
              << "        " << iB_circuit.size() << colors::reset << std::endl;
  }
}

// Test function 8: Circuit structure verification
void test_circuit_structure() {
  std::cout << "\n" << colors::bold_cyan << "=== TEST 8: Circuit Structure Verification ===" << colors::reset << std::endl;

  int lattice_size = 3;
  double interaction_strength = 1.0;

  std::cout << "\n" << colors::bold_yellow << "Detailed circuit structure for L = "
            << lattice_size << ":" << colors::reset << std::endl;

  // Test B term
  std::cout << "\n" << colors::bold_green << "B Term Circuit Gates:" << colors::reset << std::endl;
  circuit B_circuit = generate_B_term(lattice_size, interaction_strength);
  int gate_count = 0;
  for (const auto &gate : B_circuit) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate << std::endl;
    }
  }

  // Test i-B term
  std::cout << "\n" << colors::bold_blue << "i-B Term Circuit Gates:" << colors::reset << std::endl;
  circuit iB_circuit = generate_iB_term(lattice_size, interaction_strength);
  gate_count = 0;
  for (const auto &gate : iB_circuit) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate << std::endl;
    }
  }

  // Test block encoding
  std::cout << "\n" << colors::bold_magenta << "Block Encoding Circuit Gates:" << colors::reset << std::endl;
  circuit block_encoding = generate_interaction_block_encoding(lattice_size, interaction_strength);
  gate_count = 0;
  for (const auto &gate : block_encoding) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate << std::endl;
    }
  }
}

int main() {
  std::cout << colors::bold_cyan << "Running Interaction.cpp Tests" << colors::reset << std::endl;
  std::cout << colors::bold_cyan << "=============================" << colors::reset << std::endl;

  // Run all tests
  test_basic_B_term();
  test_iB_term();
  test_interaction_block_encoding();
  test_hubbard_interaction();
  test_circuit_scaling();
  test_gate_type_analysis();
  test_parameter_sensitivity();
  test_circuit_structure();

  std::cout << "\n" << colors::bold_green << "All Interaction tests completed successfully!" << colors::reset << std::endl;

  return 0;
}