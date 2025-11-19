#include "node_conversion.hpp"
#include "square_hubbard_config.hpp"
#include <bit>
#include <cassert>
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <numbers>
#include <print>
#include <set>
#include <tools_v1/tools/staq_builder.hpp>
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

using namespace tools_v1::tools;

// Block Encoding of Exp( i alpha c_{a}^\dagger c_{a} )
circuit exp_c_dag_c(qbit a, tools_v1::ast::ptr<tools_v1::ast::Expr> alpha) {
  circuit t;
  t.push_back(rz(a, std::move(alpha)));
  return t;
}

// Block Encoding of Exp( i alpha [ c_{a}^\dagger c_{b}^\dagger c_{c} c_{d} +
// c_{a} c_{b} c_{c}^\dagger c_{d} ] )
circuit exp_four_fermion(qbit a, qbit b, qbit c, qbit d,
                         tools_v1::ast::ptr<tools_v1::ast::Expr> alpha) {
  circuit t;
  tools_v1::parser::Position pos;

  // Create -2*alpha expression for the controlled rotation
  auto minus_two_alpha = tools_v1::ast::BExpr::create(
      pos, tools_v1::ast::RealExpr::create(pos, -2.0),
      tools_v1::ast::BinaryOp::Times, tools_v1::ast::object::clone(*alpha));

  // Step 1: Apply X gates to qubits a, b, d
  t.push_back(pauli_string({a.x()}));
  t.push_back(pauli_string({b.x()}));
  t.push_back(pauli_string({d.x()}));

  // Step 2: Apply CNOT gates: CNOT[d,a], CNOT[d,b], CNOT[d,c]
  t.push_back(tools_v1::ast::CNOTGate::create(pos, d.to_var_access(),
                                              a.to_var_access()));
  t.push_back(tools_v1::ast::CNOTGate::create(pos, d.to_var_access(),
                                              b.to_var_access()));
  t.push_back(tools_v1::ast::CNOTGate::create(pos, d.to_var_access(),
                                              c.to_var_access()));

  // Step 3: Apply Hadamard to qubit d
  t.push_back(hadamard(d));

  // Step 4: Apply controlled Rz(-2*alpha) with controls [a,b,c] on target d
  std::vector<tools_v1::ast::VarAccess> controls = {
      a.to_var_access(), b.to_var_access(), c.to_var_access()};
  auto rz_gate = rz(d, std::move(minus_two_alpha));

  // GateConverter rz_converter;
  // rz_gate->accept(rz_converter);

  auto multi_controlled_rz = tools_v1::ast::MultiControlGate::create(
      pos, std::move(controls), std::vector<tools_v1::ast::VarAccess>{},
      // std::move(rz_converter.converted_gate)
      tools_v1::ast::object::clone(*rz_gate));
  t.push_back(std::move(multi_controlled_rz));

  // Step 5: Apply Hadamard to qubit d
  t.push_back(hadamard(d));

  // Step 6: Apply CNOT gates in reverse: CNOT[d,c], CNOT[d,b], CNOT[d,a]
  t.push_back(tools_v1::ast::CNOTGate::create(pos, d.to_var_access(),
                                              c.to_var_access()));
  t.push_back(tools_v1::ast::CNOTGate::create(pos, d.to_var_access(),
                                              b.to_var_access()));
  t.push_back(tools_v1::ast::CNOTGate::create(pos, d.to_var_access(),
                                              a.to_var_access()));

  // Step 7: Apply X gates to qubits d, b, a
  t.push_back(pauli_string({d.x()}));
  t.push_back(pauli_string({b.x()}));
  t.push_back(pauli_string({a.x()}));

  return t;
}

// Block encoding for full kinetic term
circuit exp_kinetic(std::vector<qbit> qubits,
                    tools_v1::ast::ptr<tools_v1::ast::Expr> alpha) {
  /* input: qubits = { q0, q1, ... }
   *        alpha  = expr
   * output:
   *    product_{q in qubits}  exp(i alpha c_{q}^dagger c_{q} )
   */
  circuit t;
  // I think the result should be:
  for (auto &q : qubits) {
    auto a = tools_v1::ast::object::clone(*alpha);
    auto c = exp_c_dag_c(q, std::move(a));
    for (auto &s : c) {
      t.push_back(std::move(s));
    }
  }
  return t;
}

// Block encoding for full interacting term
circuit exp_interaction(std::vector<std::vector<qbit>> pairings,
                        tools_v1::ast::ptr<tools_v1::ast::Expr> alpha) {
  /* input: pairings = { {a0,a1,a2,a3}, {b0,b1,b2,b3}, ... }
   *        alpha    = expr
   * output:
   *    product_{p in pairings}  exp(i alpha c_{p[0]}^dagger c_{p[1]}^dagger
   * c_{p[2]} c_{p[3]} )
   */
  circuit t;
  // I think the result should be:
  for (auto &p : pairings) {
    auto a = tools_v1::ast::object::clone(*alpha);
    auto c = exp_four_fermion(p[0], p[1], p[2], p[3], std::move(a));
    for (auto &s : c) {
      t.push_back(std::move(s));
    }
  }
  return t;
}

// Test function 1: Basic exp_c_dag_c functionality
void test_exp_c_dag_c() {
  std::println("\n{}=== TEST 1: exp_c_dag_c Functionality ==={}",
               colors::bold_cyan, colors::reset);

  tools_v1::parser::Position pos;
  auto alpha = tools_v1::ast::RealExpr::create(pos, 1.5);

  std::println("\n{}exp_c_dag_c on qubit 0 with α = 1.5:{}",
               colors::bold_yellow, colors::reset);
  circuit c_dag_c = exp_c_dag_c(qbit(0), std::move(alpha));
  std::cout << colors::green << c_dag_c << colors::reset << std::endl;

  // Test with different qubit
  auto alpha2 = tools_v1::ast::RealExpr::create(pos, 0.8);
  std::println("\n{}exp_c_dag_c on qubit 3 with α = 0.8:{}",
               colors::bold_yellow, colors::reset);
  circuit c_dag_c2 = exp_c_dag_c(qbit(3), std::move(alpha2));
  std::cout << colors::blue << c_dag_c2 << colors::reset << std::endl;
}

// Test function 2: Basic exp_four_fermion functionality
void test_exp_four_fermion() {
  std::println("\n{}=== TEST 2: exp_four_fermion Functionality ==={}",
               colors::bold_cyan, colors::reset);

  tools_v1::parser::Position pos;
  auto alpha = tools_v1::ast::RealExpr::create(pos, 0.5);

  std::println("\n{}exp_four_fermion on qubits [0,1,2,3] with α = 0.5:{}",
               colors::bold_yellow, colors::reset);
  circuit four_fermion =
      exp_four_fermion(qbit(0), qbit(1), qbit(2), qbit(3), std::move(alpha));

  // Debug: Print each gate individually
  std::println("{}Debug - Individual gates in four_fermion circuit:{}",
               colors::bold_red, colors::reset);
  int gate_count = 0;
  for (const auto &gate : four_fermion) {
    if (gate) {
      std::cout << "  Gate " << gate_count++ << ": " << *gate;
    }
  }

  std::println("\n{}Full circuit output:{}", colors::bold_green, colors::reset);
  std::cout << colors::green << four_fermion << colors::reset << std::endl;

  // Test with different qubits
  auto alpha2 = tools_v1::ast::RealExpr::create(pos, 1.2);
  std::println("\n{}exp_four_fermion on qubits [4,5,6,7] with α = 1.2:{}",
               colors::bold_yellow, colors::reset);
  circuit four_fermion2 =
      exp_four_fermion(qbit(4), qbit(5), qbit(6), qbit(7), std::move(alpha2));
  std::cout << colors::blue << four_fermion2 << colors::reset << std::endl;
}

// Test function 3: Circuit analysis
void test_circuit_analysis() {
  std::println("\n{}=== TEST 3: Circuit Analysis ==={}", colors::bold_cyan,
               colors::reset);

  tools_v1::parser::Position pos;

  // Test exp_c_dag_c gate count
  std::println("\n{}exp_c_dag_c analysis:{}", colors::bold_yellow,
               colors::reset);
  auto alpha1 = tools_v1::ast::RealExpr::create(pos, 1.0);
  circuit c_dag_c = exp_c_dag_c(qbit(0), std::move(alpha1));
  std::println("{}Number of gates: {}{}", colors::green, c_dag_c.size(),
               colors::reset);
  std::println("{}Expected: 1 (single Rz gate){}", colors::blue, colors::reset);
  if (c_dag_c.size() == 1) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  // Test exp_four_fermion gate count
  std::println("\n{}exp_four_fermion analysis:{}", colors::bold_yellow,
               colors::reset);
  auto alpha2 = tools_v1::ast::RealExpr::create(pos, 1.0);
  circuit four_fermion =
      exp_four_fermion(qbit(0), qbit(1), qbit(2), qbit(3), std::move(alpha2));
  std::println("{}Number of gates: {}{}", colors::green, four_fermion.size(),
               colors::reset);
  std::println(
      "{}Expected: 13 (3 X + 6 CNOT + 2 H + 1 MultiControlGate + 1 Rz){}",
      colors::blue, colors::reset);
  if (four_fermion.size() == 13) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  // Show first few gates
  std::println("\n{}First 5 gates:{}", colors::magenta, colors::reset);
  int count = 0;
  for (const auto &gate : four_fermion) {
    if (count >= 5)
      break;
    if (gate) {
      std::cout << "  " << *gate << std::endl;
      count++;
    }
  }
  if (four_fermion.size() > 5) {
    std::println("{}... and {} more gates{}", colors::cyan,
                 four_fermion.size() - 5, colors::reset);
  }
}

// Test function 4: exp_kinetic functionality
void test_exp_kinetic() {
  std::println("\n{}=== TEST 4: exp_kinetic Functionality ==={}",
               colors::bold_cyan, colors::reset);

  tools_v1::parser::Position pos;

  // Test with multiple qubits
  std::println("\n{}exp_kinetic on qubits [0,1,2] with α = 0.5:{}",
               colors::bold_yellow, colors::reset);
  auto alpha = tools_v1::ast::RealExpr::create(pos, 0.5);
  std::vector<qbit> qubits = {qbit(0), qbit(1), qbit(2)};
  circuit kinetic = exp_kinetic(qubits, std::move(alpha));

  std::println("{}Number of gates: {}{}", colors::green, kinetic.size(),
               colors::reset);
  std::println("{}Expected: 3 (one Rz gate per qubit){}", colors::blue,
               colors::reset);
  if (kinetic.size() == 3) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  std::cout << colors::green << kinetic << colors::reset << std::endl;

  // Test with single qubit
  std::println("\n{}exp_kinetic on single qubit [5] with α = 1.2:{}",
               colors::bold_yellow, colors::reset);
  auto alpha2 = tools_v1::ast::RealExpr::create(pos, 1.2);
  std::vector<qbit> single_qubit = {qbit(5)};
  circuit kinetic_single = exp_kinetic(single_qubit, std::move(alpha2));

  std::println("{}Number of gates: {}{}", colors::green, kinetic_single.size(),
               colors::reset);
  std::println("{}Expected: 1 (single Rz gate){}", colors::blue, colors::reset);
  if (kinetic_single.size() == 1) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  std::cout << colors::blue << kinetic_single << colors::reset << std::endl;
}

// Test function 5: exp_interaction functionality
void test_exp_interaction() {
  std::println("\n{}=== TEST 5: exp_interaction Functionality ==={}",
               colors::bold_cyan, colors::reset);

  tools_v1::parser::Position pos;

  // Test with multiple pairings
  std::println("\n{}exp_interaction with two pairings and α = 0.3:{}",
               colors::bold_yellow, colors::reset);
  auto alpha = tools_v1::ast::RealExpr::create(pos, 0.3);
  std::vector<std::vector<qbit>> pairings = {
      {qbit(0), qbit(1), qbit(2), qbit(3)},
      {qbit(4), qbit(5), qbit(6), qbit(7)}};
  circuit interaction = exp_interaction(pairings, std::move(alpha));

  std::println("{}Number of gates: {}{}", colors::green, interaction.size(),
               colors::reset);
  std::println("{}Expected: 30 (15 gates per four_fermion * 2 pairings){}",
               colors::blue, colors::reset);
  if (interaction.size() == 30) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  // Show first 10 gates to avoid overwhelming output
  std::println("\n{}First 10 gates:{}", colors::magenta, colors::reset);
  int count = 0;
  for (const auto &gate : interaction) {
    if (count >= 10)
      break;
    if (gate) {
      std::cout << "  " << *gate << std::endl;
      count++;
    }
  }
  if (interaction.size() > 10) {
    std::println("{}... and {} more gates{}", colors::cyan,
                 interaction.size() - 10, colors::reset);
  }

  // Test with single pairing
  std::println("\n{}exp_interaction with single pairing and α = 0.8:{}",
               colors::bold_yellow, colors::reset);
  auto alpha2 = tools_v1::ast::RealExpr::create(pos, 0.8);
  std::vector<std::vector<qbit>> single_pairing = {
      {qbit(8), qbit(9), qbit(10), qbit(11)}};
  circuit interaction_single =
      exp_interaction(single_pairing, std::move(alpha2));

  std::println("{}Number of gates: {}{}", colors::green,
               interaction_single.size(), colors::reset);
  std::println("{}Expected: 15 (15 gates per four_fermion){}", colors::blue,
               colors::reset);
  if (interaction_single.size() == 15) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  std::cout << colors::blue << interaction_single << colors::reset << std::endl;
}

// Test function 6: Combined circuits
void test_combined_circuits() {
  std::println("\n{}=== TEST 6: Combined Circuits ==={}", colors::bold_cyan,
               colors::reset);

  tools_v1::parser::Position pos;

  std::println("\n{}Combined circuit with exp_kinetic and exp_interaction:{}",
               colors::bold_yellow, colors::reset);

  circuit combined;

  // Add exp_kinetic
  auto alpha1 = tools_v1::ast::RealExpr::create(pos, 0.7);
  std::vector<qbit> kinetic_qubits = {qbit(0), qbit(1)};
  circuit kinetic = exp_kinetic(kinetic_qubits, std::move(alpha1));
  for (const auto &gate : kinetic) {
    if (gate) {
      combined.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  // Add exp_interaction
  auto alpha2 = tools_v1::ast::RealExpr::create(pos, 0.3);
  std::vector<std::vector<qbit>> interaction_pairings = {
      {qbit(2), qbit(3), qbit(4), qbit(5)}};
  circuit interaction =
      exp_interaction(interaction_pairings, std::move(alpha2));
  for (const auto &gate : interaction) {
    if (gate) {
      combined.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  std::println("{}Total gates in combined circuit: {}{}", colors::green,
               combined.size(), colors::reset);
  std::println("{}Expected: 17 (2 from kinetic + 15 from interaction){}",
               colors::blue, colors::reset);
  if (combined.size() == 17) {
    std::println("{}✓ Gate count matches expected{}", colors::bold_green,
                 colors::reset);
  } else {
    std::println("{}✗ Gate count mismatch{}", colors::bold_red, colors::reset);
  }

  std::cout << colors::cyan << combined << colors::reset << std::endl;
}

// Test function 7: Time evolution of ground state
void test_time_evolution_ground_state() {
  std::println("\n{}=== TEST 7: Time Evolution of Ground State ==={}",
               colors::bold_cyan, colors::reset);

  tools_v1::parser::Position pos;

  // Create L=6 lattice configuration
  const int L = 6;
  square_hubbard_config HC(L, 1.0, 0.0);
  int L_int = static_cast<int>(L);

  std::println("\n{}L = {} lattice configuration:{}", colors::bold_yellow, L,
               colors::reset);

  // Create ground state preparation (similar to GS.cpp)
  double mu = -1.5;
  circuit ground_state;
  std::set<int> selected_qubits;

  for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
      if (HC.e_bare(x, y) <= mu) {
        int n = HC.encoding_formula(x, y);
        selected_qubits.insert(n);
        qbit q(n);
        ground_state.push_back(pauli_string({q.x()}));
      }
    }
  }

  std::print("{}Ground state selected qubits: {}", colors::green,
             colors::reset);
  for (auto &x : selected_qubits)
    std::print("{}{}{} ", colors::bold_blue, x, colors::reset);
  std::println("");

  std::println("{}Ground state preparation circuit: {}", colors::bold_yellow,
               colors::reset);
  std::cout << colors::green << ground_state << colors::reset << std::endl;

  // Create kinetic term acting on all lattice sites
  std::println("\n{}Kinetic term acting on all lattice sites:{}",
               colors::bold_yellow, colors::reset);

  std::vector<qbit> all_qubits;
  for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
      int n = HC.encoding_formula(x, y);
      all_qubits.emplace_back(n);
    }
  }

  auto kinetic_alpha = tools_v1::ast::RealExpr::create(pos, 0.1);
  circuit kinetic_term = exp_kinetic(all_qubits, std::move(kinetic_alpha));

  std::println("{}Kinetic term gates: {}{}", colors::green, kinetic_term.size(),
               colors::reset);
  std::println("{}Expected: {} (one Rz gate per site){}", colors::blue,
               all_qubits.size(), colors::reset);

  // Show first 5 gates of kinetic term
  std::println("\n{}First 5 gates of kinetic term:{}", colors::magenta,
               colors::reset);
  int count = 0;
  for (const auto &gate : kinetic_term) {
    if (count >= 5)
      break;
    if (gate) {
      std::cout << "  " << *gate << std::endl;
      count++;
    }
  }
  if (kinetic_term.size() > 5) {
    std::println("{}... and {} more gates{}", colors::cyan,
                 kinetic_term.size() - 5, colors::reset);
  }

  // Create interaction terms for ALL possible quadruples
  std::println("\n{}Generating ALL possible interaction quadruples:{}",
               colors::bold_yellow, colors::reset);

  std::vector<std::vector<qbit>> interaction_pairings;

  // Get all lattice site encodings
  std::vector<int> all_encodings;
  for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
      all_encodings.push_back(HC.encoding_formula(x, y));
    }
  }

  std::println("{}Total lattice sites: {}{}", colors::green,
               all_encodings.size(), colors::reset);

  // Generate all possible quadruples (a,b,c,d) with a < b and c < d
  // This avoids duplicates from permutations
  int quadruple_count = 0;
  for (size_t i = 0; i < all_encodings.size(); ++i) {
    for (size_t j = i + 1; j < all_encodings.size(); ++j) {
      for (size_t k = 0; k < all_encodings.size(); ++k) {
        for (size_t l = k + 1; l < all_encodings.size(); ++l) {
          // Ensure all four sites are distinct
          if (i != k && i != l && j != k && j != l) {
            interaction_pairings.push_back(
                {qbit(all_encodings[i]), qbit(all_encodings[j]),
                 qbit(all_encodings[k]), qbit(all_encodings[l])});
            quadruple_count++;
          }
        }
      }
    }
  }

  std::println("{}Generated {} unique quadruples{}", colors::blue,
               quadruple_count, colors::reset);

  auto interaction_alpha = tools_v1::ast::RealExpr::create(pos, 0.05);
  circuit interaction_term =
      exp_interaction(interaction_pairings, std::move(interaction_alpha));

  std::println("{}Interaction term gates: {}{}", colors::green,
               interaction_term.size(), colors::reset);
  std::println("{}Expected: {} (15 gates per quadruple * {} quadruples){}",
               colors::blue, 15 * interaction_pairings.size(),
               interaction_pairings.size(), colors::reset);

  // Combine ground state + kinetic + interaction for time evolution
  std::println("\n{}Complete time evolution circuit:{}", colors::bold_yellow,
               colors::reset);

  circuit time_evolution;

  // Add ground state preparation
  for (const auto &gate : ground_state) {
    if (gate) {
      time_evolution.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  // Add kinetic term
  for (const auto &gate : kinetic_term) {
    if (gate) {
      time_evolution.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  // Add interaction term
  for (const auto &gate : interaction_term) {
    if (gate) {
      time_evolution.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  std::println("{}Total gates in time evolution circuit: {}{}", colors::green,
               time_evolution.size(), colors::reset);
  std::println(
      "{}Expected: {} (ground state) + {} (kinetic) + {} (interaction){}",
      colors::blue, ground_state.size(), kinetic_term.size(),
      interaction_term.size(), colors::reset);

  // Show summary of the complete circuit
  std::println("\n{}Time evolution circuit summary:{}", colors::bold_magenta,
               colors::reset);
  std::println("  {}Ground state preparation: {} gates{}", colors::green,
               ground_state.size(), colors::reset);
  std::println("  {}Kinetic term: {} gates{}", colors::blue,
               kinetic_term.size(), colors::reset);
  std::println("  {}Interaction term: {} gates{}", colors::cyan,
               interaction_term.size(), colors::reset);
  std::println("  {}Total: {} gates{}", colors::bold_yellow,
               time_evolution.size(), colors::reset);
}

// Test function 8: Detailed lattice analysis for L=6
void test_lattice_analysis_L6() {
  std::println("\n{}=== TEST 8: L=6 Lattice Analysis ==={}", colors::bold_cyan,
               colors::reset);

  const int L = 6;
  square_hubbard_config HC(L, 1.0, 0.0);
  int L_int = static_cast<int>(L);

  std::println("\n{}L = {} lattice encoding and energies:{}",
               colors::bold_yellow, L, colors::reset);

  // Print encoding grid
  for (int r = L_int / 2; r >= -L_int / 2 + 1; --r) {
    for (int c = -L_int / 2 + 1; c <= L_int / 2; ++c) {
      int encoding = HC.encoding_formula(c, r);
      double energy = HC.e_bare(c, r);
      std::print("{}[{:2d}:{:6.3f}]{} ", colors::cyan, encoding, energy,
                 colors::reset);
    }
    std::println("");
  }

  // Show which sites would be selected for ground state
  double mu = -1.5;
  std::println("\n{}Ground state selection for μ = {:.1f}:{}",
               colors::bold_yellow, mu, colors::reset);

  for (int r = L_int / 2; r >= -L_int / 2 + 1; --r) {
    for (int c = -L_int / 2 + 1; c <= L_int / 2; ++c) {
      double energy = HC.e_bare(c, r);
      if (energy <= mu) {
        std::print("{}[{:2d}:{:6.3f}]{} ", colors::bold_green,
                   HC.encoding_formula(c, r), energy, colors::reset);
      } else {
        std::print("{}[{:2d}:{:6.3f}]{} ", colors::bold_red, HC.encoding_formula(c, r),
                   energy, colors::reset);
      }
    }
    std::println("");
  }

  // Count total number of possible interaction quadruples
  std::println("\n{}Interaction quadruple analysis:{}", colors::bold_yellow,
               colors::reset);

  int total_sites = L_int * L_int;
  std::println("{}Total lattice sites: {}{}", colors::green, total_sites,
               colors::reset);

  // For a real Hubbard model, the number of interaction terms is huge
  // (all combinations of 4 distinct sites)
  long long possible_quadruples = total_sites * (total_sites - 1) *
                                  (total_sites - 2) * (total_sites - 3) / 24;
  std::println("{}Possible interaction quadruples: {}{}", colors::blue,
               possible_quadruples, colors::reset);
  std::println(
      "{}Note: This is the total number of possible 4-site interactions{}",
      colors::magenta, colors::reset);
}

// Test function 9: Write complete time evolution circuit to file
void test_write_time_evolution_to_file() {
  std::println("\n{}=== TEST 9: Writing Time Evolution to File ==={}",
               colors::bold_cyan, colors::reset);

  tools_v1::parser::Position pos;

  // Create L=6 lattice configuration
  const int L = 6;
  square_hubbard_config HC(L, 1.0, 0.4);
  int L_int = static_cast<int>(L);

  std::println(
      "\n{}Creating complete time evolution circuit for L={} lattice{}",
      colors::bold_yellow, L, colors::reset);

  // Create ground state preparation
  double mu = -1.5;
  circuit ground_state;
  std::set<int> selected_qubits;

  for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
      if (HC.e_bare(x, y) <= mu) {
        int n = HC.encoding_formula(x, y);
        selected_qubits.insert(n);
        qbit q(n);
        ground_state.push_back(pauli_string({q.x()}));
      }
    }
  }

  // Create kinetic term acting on all lattice sites
  std::vector<qbit> all_qubits;
  for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
      int n = HC.encoding_formula(x, y);
      all_qubits.emplace_back(n);
    }
  }

  auto kinetic_alpha = tools_v1::ast::RealExpr::create(pos, 0.1);
  circuit kinetic_term = exp_kinetic(all_qubits, std::move(kinetic_alpha));

  // Create interaction terms for ALL possible quadruples
  std::vector<std::vector<qbit>> interaction_pairings;

  // Get all lattice site encodings
  std::vector<int> all_encodings;
  for (int x = -L_int / 2 + 1; x <= L_int / 2; ++x) {
    for (int y = -L_int / 2 + 1; y <= L_int / 2; ++y) {
      all_encodings.push_back(HC.encoding_formula(x, y));
    }
  }

  // Generate all possible quadruples (a,b,c,d) with a < b and c < d
  int quadruple_count = 0;
  for (size_t i = 0; i < all_encodings.size(); ++i) {
    for (size_t j = i + 1; j < all_encodings.size(); ++j) {
      for (size_t k = 0; k < all_encodings.size(); ++k) {
        for (size_t l = k + 1; l < all_encodings.size(); ++l) {
          // Ensure all four sites are distinct
          if (i != k && i != l && j != k && j != l) {
            interaction_pairings.push_back(
                {qbit(all_encodings[i]), qbit(all_encodings[j]),
                 qbit(all_encodings[k]), qbit(all_encodings[l])});
            quadruple_count++;
          }
        }
      }
    }
  }

  auto interaction_alpha = tools_v1::ast::RealExpr::create(pos, 0.05);
  circuit interaction_term =
      exp_interaction(interaction_pairings, std::move(interaction_alpha));

  // Combine all components for time evolution
  circuit time_evolution;

  // Add ground state preparation
  for (const auto &gate : ground_state) {
    if (gate) {
      time_evolution.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  // Add kinetic term
  for (const auto &gate : kinetic_term) {
    if (gate) {
      time_evolution.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  // Add interaction term
  for (const auto &gate : interaction_term) {
    if (gate) {
      time_evolution.push_back(tools_v1::ast::object::clone(*gate));
    }
  }

  std::println("{}Circuit statistics:{}", colors::bold_magenta, colors::reset);
  std::println("  {}Ground state preparation: {} gates{}", colors::green,
               ground_state.size(), colors::reset);
  std::println("  {}Kinetic term: {} gates{}", colors::blue,
               kinetic_term.size(), colors::reset);
  std::println("  {}Interaction term: {} gates ({} quadruples){}", colors::cyan,
               interaction_term.size(), quadruple_count, colors::reset);
  std::println("  {}Total: {} gates{}", colors::bold_yellow,
               time_evolution.size(), colors::reset);

  // Write circuit to file
  std::string filename = "time_evolution.qasm";
  std::ofstream file(filename);
  if (file.is_open()) {
    file << time_evolution;
    file.close();
    std::println("\n{}✓ Successfully wrote circuit to {}{}", colors::bold_green,
                 filename, colors::reset);
    std::println("{}File size: {} gates{}", colors::blue, time_evolution.size(),
                 colors::reset);
  } else {
    std::println("\n{}✗ Failed to open file {}{}", colors::bold_red, filename,
                 colors::reset);
  }
}

int main() {
  std::println("{}Running TrotterHam1.cpp Tests{}", colors::bold_cyan,
               colors::reset);
  std::println("{}=========================={}", colors::bold_cyan,
               colors::reset);

  // Run all tests
  test_exp_c_dag_c();
  test_exp_four_fermion();
  test_circuit_analysis();
  test_exp_kinetic();
  test_exp_interaction();
  test_combined_circuits();
  test_time_evolution_ground_state();
  test_lattice_analysis_L6();
  test_write_time_evolution_to_file();

  return 0;
}
