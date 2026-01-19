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

// Input: diagonal matrix D = diag(D_0, D_1, ..., D_n) // assume n is power of 2
// 1. convert each of D_i -> binary D_i0, D_i1, ... D_il
// 2. create a multicontrolled gate that controls on i and flips the encoding qubits according to binary rep of D_i
//       eg. i = 0, control [] :: [0,1,2,..] :: PauliString X[a] X[b] X[c];
//       eg. i = 1, control [0] :: [1,2,..] :: PauliString X[e] X[f] X[g];
//       eg. i = 2, control [1] :: [0,2,..] :: PauliString X[alpha] X[beta] X[gamma];
//       eg. i = 3, control [0,1] :: [2,..] :: PauliString X[zeta] X[rho] X[mu];
//       ...
// 3. aggregate these multicontrolled gates together to get a circuit OD
// 4. compute alphaDprime = (min_i D_i)^{-1}
// 5. Construct INV circuit that implements: INV|zeta>|0> = |zeta>( \frac{1}{alphaDprime zeta} |0> + (...)|1> )


int main (int argc, char *argv[]) {
  return 0;
}
