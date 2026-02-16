#ifndef TOOLS_V1_UTILS_HPP_
#define TOOLS_V1_UTILS_HPP_

#include <tools_v1/tools/staq_builder.hpp>

namespace tools_v1::algorithm {

using namespace tools_v1::tools;

// Create a rotation gate R_Y(θ) = [[cos(θ/2), -sin(θ/2)], [sin(θ/2), cos(θ/2)]]
inline tools_v1::ast::ptr<tools_v1::ast::Gate> ry_gate(double angle,
                                                const qbit &target) {
  tools_v1::parser::Position pos;
  auto angle_expr = tools_v1::ast::RealExpr::create(pos, angle);
  auto target_qubit = target.to_va();

  // Use DeclaredGate with "ry" for rotation around Y axis
  std::vector<tools_v1::ast::ptr<tools_v1::ast::Expr>> args;
  args.push_back(std::move(angle_expr));
  return tools_v1::ast::DeclaredGate::create(
      pos, "ry", std::move(args),
      std::vector<tools_v1::ast::VarAccess>{target_qubit});
}

// Create a rotation gate R_Z(μ)
inline tools_v1::ast::ptr<tools_v1::ast::Gate> rz_gate(double angle,
                                                const qbit &target) {
  tools_v1::parser::Position pos;
  auto angle_expr = tools_v1::ast::RealExpr::create(pos, angle);
  auto target_qubit = target.to_va();

  std::vector<tools_v1::ast::ptr<tools_v1::ast::Expr>> args;
  args.push_back(std::move(angle_expr));
  return tools_v1::ast::DeclaredGate::create(
      pos, "rz", std::move(args),
      std::vector<tools_v1::ast::VarAccess>{target_qubit});
}

} // namespace tools_v1::algorithm

#endif
