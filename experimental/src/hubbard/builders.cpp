#include <hubbard/builders.hpp>

#include <vector>

namespace hubbard {

using tools_v1::ast::VarAccess;
using tools_v1::tools::circuit;
using tools_v1::tools::qbit;

tools_v1::tools::circuit build_creation(int idx, BuildContext &ctx) {
  circuit c;
  qbit ancilla = ctx.anc_mem.generate_ancilla("creation");

  std::vector<VarAccess> cnot_qargs;
  cnot_qargs.emplace_back(ctx.data[idx].to_va());
  cnot_qargs.emplace_back(ancilla.to_va());

  auto xg1 = tools_v1::ast::DeclaredGate::create(ctx.pos, "x", {},
                                                 {ctx.data[idx].to_va()});
  auto cnt = tools_v1::ast::DeclaredGate::create(ctx.pos, "cx", {},
                                                 std::move(cnot_qargs));
  auto xg2 = tools_v1::ast::DeclaredGate::create(ctx.pos, "x", {},
                                                 {ctx.data[idx].to_va()});

  c.push_back(std::move(xg1));
  c.push_back(std::move(cnt));
  c.push_back(std::move(xg2));

  for (int i = 0; i < idx; ++i) {
    auto z_gate = tools_v1::ast::DeclaredGate::create(ctx.pos, "z", {},
                                                      {ctx.data[i].to_va()});
    c.push_back(std::move(z_gate));
  }

  auto x3 = tools_v1::ast::DeclaredGate::create(ctx.pos, "x", {},
                                                {ctx.data[idx].to_va()});
  c.push_back(std::move(x3));

  return c;
}

tools_v1::tools::circuit build_annihilation(int idx, BuildContext &ctx) {
  circuit c;
  qbit ancilla = ctx.anc_mem.generate_ancilla("annihilation");

  std::vector<VarAccess> cnot_qargs;
  cnot_qargs.emplace_back(ctx.data[idx].to_va());
  cnot_qargs.emplace_back(ancilla.to_va());

  auto cnt = tools_v1::ast::DeclaredGate::create(ctx.pos, "cx", {},
                                                 std::move(cnot_qargs));
  c.push_back(std::move(cnt));

  for (int i = 0; i < idx; ++i) {
    auto z_gate = tools_v1::ast::DeclaredGate::create(ctx.pos, "z", {},
                                                      {ctx.data[i].to_va()});
    c.push_back(std::move(z_gate));
  }

  auto x_gate = tools_v1::ast::DeclaredGate::create(ctx.pos, "x", {},
                                                    {ctx.data[idx].to_va()});
  c.push_back(std::move(x_gate));

  return c;
}

} // namespace hubbard
