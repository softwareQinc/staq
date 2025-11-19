#include <hubbard/program_io.hpp>

#include <fstream>
#include <sstream>
#include <resource_estimator.hpp>
#include <to_qasm_converter.hpp>

namespace hubbard {

using tools_v1::ast::Program;
using tools_v1::ast::RegisterDecl;
using tools_v1::tools::circuit;

void push_gate_builder(tools_v1::ast::ptr<Program> &prog,
                       tools_v1::ast::GateBuilder &gb) {
  prog->body().splice(prog->end(), gb.submit_list());
}

void push_circuit(tools_v1::ast::ptr<Program> &prog, circuit &c) {
  prog->body().splice(prog->end(), c.body_list());
}

void materialize_registers(Program &prog,
                           const tools_v1::tools::ANC_MEM &anc_mem,
                           const std::string &data_reg_name,
                           int num_data_qubits) {
  tools_v1::parser::Position pos;

  auto &registers =
      const_cast<tools_v1::tools::ANC_MEM &>(anc_mem).registers();
  for (const auto &[name, idx] : registers) {
    auto reg = RegisterDecl::create(pos, name, true, idx + 1);
    prog.body().push_back(std::move(reg));
  }

  auto data_reg =
      RegisterDecl::create(pos, data_reg_name, true, num_data_qubits);
  prog.body().push_back(std::move(data_reg));
}

void save_program(const Program &prog, const std::string &path) {
  std::ofstream os(path);
  prog.pretty_print(os);
}

void save_qasm(const std::string &path, std::string_view contents) {
  std::ofstream os(path);
  os.write(contents.data(),
           static_cast<std::streamsize>(contents.size()));
}

QasmArtifacts qasmify_program(Program &prog) {
  tools_v1::ast::QASMify to_qasm;
  prog.accept(to_qasm);

  std::ostringstream oss;
  to_qasm.pretty_print(oss);

  QasmArtifacts artifacts;
  artifacts.code = oss.str();
  auto &qasm_prog = to_qasm.prog();
  artifacts.program = std::move(qasm_prog);
  return artifacts;
}

std::map<std::string, double>
estimate_resources(const Program &prog,
                   std::optional<double> rotation_precision) {
  tools_v1::tools::ResourceEstimator::config cfg;
  if (rotation_precision) {
    cfg.rotation_precision = rotation_precision;
  }
  auto counts =
      tools_v1::tools::estimate_resources(const_cast<Program &>(prog), cfg);
  std::map<std::string, double> result;
  for (const auto &[key, value] : counts) {
    result.emplace(key, value);
  }
  return result;
}

} // namespace hubbard
