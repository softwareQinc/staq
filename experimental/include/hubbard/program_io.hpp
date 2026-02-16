#ifndef HUBBARD_PROGRAM_IO_HPP_
#define HUBBARD_PROGRAM_IO_HPP_

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tools_v1/ast/gate_builder_simple.hpp>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>

namespace hubbard {

struct QasmArtifacts {
  std::string code;
  tools_v1::ast::ptr<tools_v1::ast::Program> program;
};

void push_gate_builder(tools_v1::ast::ptr<tools_v1::ast::Program> &prog,
                       tools_v1::ast::GateBuilder &gb);
void push_circuit(tools_v1::ast::ptr<tools_v1::ast::Program> &prog,
                  tools_v1::tools::circuit &circuit);

void materialize_registers(tools_v1::ast::Program &prog,
                           const tools_v1::tools::ANC_MEM &anc_mem,
                           const std::string &data_reg_name,
                           int num_data_qubits);

void save_program(const tools_v1::ast::Program &prog,
                  const std::string &path);
void save_qasm(const std::string &path, std::string_view contents);

QasmArtifacts qasmify_program(tools_v1::ast::Program &prog);
std::map<std::string, double> estimate_resources(
    const tools_v1::ast::Program &prog,
    std::optional<double> rotation_precision = std::nullopt);

} // namespace hubbard

#endif // HUBBARD_PROGRAM_IO_HPP_
