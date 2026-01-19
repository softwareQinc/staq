#ifndef MULTIPLICATION_HPP_
#define MULTIPLICATION_HPP_

#include <tools_v1/tools/staq_builder.hpp>

namespace tools_v1 {
namespace algorithm {

inline tools::circuit circuit_combine(const tools::circuit& u1,
                                      const tools::circuit& u2) {
  tools::circuit c;
  //
  for (const auto &x : u1)
    c.push_back(ast::object::clone(*x));
  for (const auto &x : u2)
    c.push_back(ast::object::clone(*x));
  //
  for (auto it = u1.ancilla_begin(); it != u1.ancilla_end(); ++it)
    c.save_ancilla(**it);
  for (auto it = u2.ancilla_begin(); it != u2.ancilla_end(); ++it)
    c.save_ancilla(**it);
  return c;
}

inline tools::circuit circuit_combine(
    const std::vector<tools::circuit> &unitaries){
  tools::circuit c;
  //
  for(const auto &u : unitaries){
    for (const auto &x : u)
      c.push_back(ast::object::clone(*x));
    for (auto it = u.ancilla_begin(); it != u.ancilla_end(); ++it)
      c.save_ancilla(**it);
  }
  return c;
}


} // namespace algorithm
} // namespace tools_v1

#endif
