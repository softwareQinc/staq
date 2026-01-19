#include <bit>
#include <cassert>
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <print>
#include <tools_v1/tools/staq_builder.hpp>
#include <vector>
#include <set>
#include "square_hubbard_config.hpp"

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
}

using namespace tools_v1::tools;

circuit gs_guess(double mu, square_hubbard_config &HC) {
  circuit gs_prep;
  auto L = HC.L();

  int L_int = static_cast<int>(L);
  assert(L_int % 2 == 0);

  std::set<int> s;
  for (int x = -L_int / 2 + 1; x <= L_int / 2-1; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2-1; ++y) {
      if (HC.e_bare(x, y) <= mu) {
        int n = HC.encoding_formula(x, y);
        s.insert(n);
        qbit q(n);
        gs_prep.push_back(pauli_string({q.x()}));
      }
    }
  }
  std::print("{}Selected qubits: {}", colors::green, colors::reset);
  for(auto& x : s)
    std::print("{}{}{} ", colors::bold_blue, x, colors::reset);
  std::println("");

  return gs_prep;
}

// Test function 1: Print encodings for various lattice sizes
void test_encodings() {
  std::println("\n{}=== TEST 1: Encoding Verification ==={}", colors::bold_cyan, colors::reset);

  // Test small lattice (L=3)
  std::println("\n{}L = 3 (3x3 grid):{}", colors::bold_yellow, colors::reset);
  square_hubbard_config HC3(3, 1.0, 0.0);
  for(int r = 1; r >= -1; --r){
    for(int c = -1; c <= 1; ++c){
      std::print("{}{:2}{}    ", colors::green, HC3.encoding_formula(c, r), colors::reset);
    }
    std::println("");
  }

  // Test medium lattice (L=5)
  std::println("\n{}L = 5 (5x5 grid center):{}", colors::bold_yellow, colors::reset);
  square_hubbard_config HC5(5, 1.0, 0.0);
  for(int r = 2; r >= -2; --r){
    for(int c = -2; c <= 2; ++c){
      std::print("{}{:3}{}   ", colors::blue, HC5.encoding_formula(c, r), colors::reset);
    }
    std::println("");
  }

  // Test large lattice (L=7) - just show pattern
  std::println("\n{}L = 7 (7x7 grid center):{}", colors::bold_yellow, colors::reset);
  square_hubbard_config HC7(7, 1.0, 0.0);
  for(int r = 3; r >= -3; --r){
    for(int c = -3; c <= 3; ++c){
      std::print("{}{:3}{}\t", colors::magenta, HC7.encoding_formula(c, r), colors::reset);
    }
    std::println("");
  }
}

// Test function 2: Show excitations for different chemical potentials
void test_excitations() {
  std::println("\n{}=== TEST 2: Excitation Selection ==={}", colors::bold_cyan, colors::reset);

  const int L = 4;
  square_hubbard_config HC(L, 1.0, 0.0);
  int L_int = static_cast<int>(L);

  // Test different chemical potentials
  std::vector<double> mu_values = {-3.0, -2.0, -1.0, 0.0, 1.0, 2.0};

  for(double mu : mu_values) {
    std::println("\n{}Chemical potential μ = {}{}", colors::bold_yellow, mu, colors::reset);

    std::set<int> selected_qubits;
    for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
      for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
        if (HC.e_bare(x, y) <= mu) {
          int n = HC.encoding_formula(x, y);
          selected_qubits.insert(n);
        }
      }
    }

    std::print("{}Selected qubits: {}", colors::green, colors::reset);
    for(int q : selected_qubits) {
      std::print("{}{}{} ", colors::bold_blue, q, colors::reset);
    }
    std::println("{} ({}{}{} total){}", colors::green, colors::bold_red, selected_qubits.size(), colors::reset, colors::reset);

    // Show corresponding coordinates
    std::print("{}Corresponding coordinates: {}", colors::green, colors::reset);
    for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
      for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
        if (HC.e_bare(x, y) <= mu) {
          std::print("{} ({},{}) {}", colors::magenta, x, y, colors::reset);
        }
      }
    }
    std::println("");
  }
}

// Test function 3: Verify specific known patterns
void test_known_patterns() {
  std::println("\n{}=== TEST 3: Known Pattern Verification ==={}", colors::bold_cyan, colors::reset);

  square_hubbard_config HC(3, 1.0, 0.0);

  // Test specific coordinates that should match Figure 6
  std::vector<std::pair<int, int>> test_coords = {
    {-1, 1}, {0, 1}, {1, 1},
    {-1, 0}, {0, 0}, {1, 0},
    {-1, -1}, {0, -1}, {1, -1}
  };

  std::vector<int> expected = {8, 2, 6, 3, 0, 1, 10, 4, 12};

  std::println("{}Expected pattern from Figure 6:{}", colors::bold_yellow, colors::reset);
  for(size_t i = 0; i < test_coords.size(); ++i) {
    int x = test_coords[i].first;
    int y = test_coords[i].second;
    int actual = HC.encoding_formula(x, y);
    std::print("{} ({},{}) -> {}{}", colors::blue, x, y, colors::bold_magenta, actual);
    if(actual == expected[i]) {
      std::println(" {}✓{}", colors::bold_green, colors::reset);
    } else {
      std::println(" {}✗ (expected {}){}", colors::bold_red, expected[i], colors::reset);
    }
  }
}

// Test function 4: Print grid with e_bare values instead of encoding
void test_energy_grid() {
  std::println("\n{}=== TEST 4: Energy Grid Visualization ==={}", colors::bold_cyan, colors::reset);

  // Test with L=3
  std::println("\n{}L = 3 (3x3 grid) - e_bare values:{} ", colors::bold_yellow, colors::reset);
  square_hubbard_config HC3(3, 1.0, 0.0);
  for(int r = 1; r >= -1; --r){
    for(int c = -1; c <= 1; ++c){
      double energy = HC3.e_bare(c, r);
      std::print("{}{:6.3f}{} ", colors::green, energy, colors::reset);
    }
    std::println("");
  }

  // Test with L=5
  std::println("\n{}L = 5 (5x5 grid center) - e_bare values:{} ", colors::bold_yellow, colors::reset);
  square_hubbard_config HC5(5, 1.0, 0.0);
  for(int r = 2; r >= -2; --r){
    for(int c = -2; c <= 2; ++c){
      double energy = HC5.e_bare(c, r);
      std::print("{}{:6.3f}{} ", colors::blue, energy, colors::reset);
    }
    std::println("");
  }

  // Test with L=7
  std::println("\n{}L = 7 (7x7 grid center) - e_bare values:{} ", colors::bold_yellow, colors::reset);
  square_hubbard_config HC7(7, 1.0, 0.0);
  for(int r = 3; r >= -3; --r){
    for(int c = -3; c <= 3; ++c){
      double energy = HC7.e_bare(c, r);
      std::print("{}{:6.3f}{} ", colors::magenta, energy, colors::reset);
    }
    std::println("");
  }
}

// Test function 5: Colored grid showing ground state selection for various mu values
void test_ground_state_selection_grid() {
  std::println("\n{}=== TEST 5: Ground State Selection Grid ==={}", colors::bold_cyan, colors::reset);

  const int L = 16;
  square_hubbard_config HC(L, 1.0, 0.0);
  int L_int = static_cast<int>(L);

  // Test different chemical potentials
  std::vector<double> mu_values = {-3.0, -2.0, -1.0, 0.0, 1.0, 2.0};

  for(double mu : mu_values) {
    std::println("\n{}Chemical potential μ = {:.1f}{}", colors::bold_yellow, mu, colors::reset);
    std::println("{}Legend: {}●{} = selected (e_bare ≤ μ), {}○{} = not selected{}",
                 colors::green, colors::bold_green, colors::green, colors::bold_red, colors::green, colors::reset);

    for(int r = L_int / 2 -1; r >= -L_int / 2 + 1; --r){
      for(int c = -L_int / 2 + 1; c <= L_int / 2-1; ++c){
        double energy = HC.e_bare(c, r);
        if (energy <= mu) {
          // Selected for ground state - green filled circle
          std::print("{}●{} ", colors::bold_green, colors::reset);
        } else {
          // Not selected - red hollow circle
          std::print("{}○{} ", colors::bold_red, colors::reset);
        }
      }
      std::println("");
    }

    // Show coordinates and energies for selected qubits
    std::print("{}Selected coordinates (e_bare ≤ μ): {}", colors::green, colors::reset);
    for (int x = -L_int / 2 + 1; x <= L_int / 2 - 1; ++x) {
      for (int y = -L_int / 2 + 1; y <= L_int / 2 - 1; ++y) {
        if (HC.e_bare(x, y) <= mu) {
          double energy = HC.e_bare(x, y);
          std::print("{} ({},{}) [{:.3f}] {}", colors::magenta, x, y, energy, colors::reset);
        }
      }
    }
    std::println("");
  }
}

// Test function 6: Detailed energy and selection analysis
void test_detailed_energy_analysis() {
  std::println("\n{}=== TEST 6: Detailed Energy Analysis ==={}", colors::bold_cyan, colors::reset);

  const int L = 10;
  square_hubbard_config HC(L, 1.0, 0.0);
  int L_int = static_cast<int>(L);

  std::println("{}L = {} grid:{} ", colors::bold_yellow, L, colors::reset);

  // Print header
  std::print("{}     ", colors::reset);
  for(int c = -L_int / 2 + 1; c <= L_int / 2; ++c){
    std::print("{:>8} ", c);
  }
  std::println("");

  // Print grid with both encoding and energy
  for(int r = L_int / 2 - 1; r >= -L_int / 2 + 1; --r){
    std::print("{:>3} | ", r);
    for(int c = -L_int / 2 + 1; c <= L_int / 2 -1; ++c){
      int encoding = HC.encoding_formula(c, r);
      double energy = HC.e_bare(c, r);
      std::print("{}[{:2d}:{:6.3f}]{} ", colors::cyan, encoding, energy, colors::reset);
    }
    std::println("");
  }

  // Show selection for different mu values
  std::vector<double> mu_values = {-2.5, -1.5, -0.5, 0.5, 1.5};
  for(double mu : mu_values) {
    std::println("\n{}μ = {:.1f}:{}", colors::bold_yellow, mu, colors::reset);

    for(int r = L_int / 2-1; r >= -L_int / 2 + 1; --r){
      for(int c = -L_int / 2 + 1; c <= L_int / 2-1; ++c){
        double energy = HC.e_bare(c, r);
        if (energy <= mu) {
          std::print("{}[{:2d}:{:6.3f}]{} ", colors::bold_green, HC.encoding_formula(c, r), energy, colors::reset);
        } else {
          std::print("{}[{:2d}:{:6.3f}]{} ", colors::bold_red, HC.encoding_formula(c, r), energy, colors::reset);
        }
      }
      std::println("");
    }
  }
}

int main() {
  std::println("{}Running GS.cpp Tests{}", colors::bold_cyan, colors::reset);
  std::println("{}===================={}", colors::bold_cyan, colors::reset);

  // Run all tests
  test_encodings();
  test_excitations();
  test_known_patterns();
  test_energy_grid();
  test_ground_state_selection_grid();
  test_detailed_energy_analysis();

  // Original functionality
  std::println("\n{}=== Original Functionality ==={}", colors::bold_cyan, colors::reset);
  const int L = 10;
  square_hubbard_config HC(L, 1.0, 0.0);
  double mu = -1.5;
  circuit ground_state = gs_guess(mu, HC);
  std::println("{}Ground state preparation circuit for μ = {}{}", colors::bold_yellow, mu, colors::reset);
  std::cout << colors::green << ground_state << colors::reset << std::endl;

  return 0;
}
