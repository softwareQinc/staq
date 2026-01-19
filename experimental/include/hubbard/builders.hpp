#ifndef HUBBARD_BUILDERS_HPP_
#define HUBBARD_BUILDERS_HPP_

#include <span>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>

namespace hubbard {

struct BuildContext {
  tools_v1::parser::Position &pos;
  std::span<tools_v1::tools::qbit> data;
  tools_v1::tools::ANC_MEM &anc_mem;
};

tools_v1::tools::circuit build_creation(int idx, BuildContext &ctx);
tools_v1::tools::circuit build_annihilation(int idx, BuildContext &ctx);

} // namespace hubbard

#endif // HUBBARD_BUILDERS_HPP_
