#include "utils/angle.hpp"

using namespace synthewareQ::utils;

int main() {
  std::cout << angles::pi_quarter << "\n";
  std::cout << angles::pi/4 << "\n";
  std::cout << angles::pi << "\n";
  std::cout << angles::zero << "\n";
  std::cout << Angle(-1, 4) << "\n";
  std::cout << Angle(-1, 4) - angles::pi_half << "\n";
  std::cout << Angle(-1, 4) + Angle(3.1415) << "\n";
}
