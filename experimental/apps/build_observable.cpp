#include <circuit_dagger.hpp>
#include <complex>
#include <filesystem>
#include <hubbard/builders.hpp>
#include <hubbard/layout.hpp>
#include <hubbard/model_params.hpp>
#include <hubbard/operators.hpp>
#include <hubbard/program_io.hpp>
#include <iostream>
#include <list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tools_v1/algorithm/LCU.hpp>
#include <tools_v1/algorithm/Multiplication.hpp>
#include <tools_v1/algorithm/QSVT.hpp>
#include <tools_v1/ast/gate_builder_simple.hpp>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>

using qbit = tools_v1::tools::qbit;

namespace {

struct CliOptions {
  unsigned ell = 7;
  double t = 1.0;
  double U = 4.0;
  double E0 = 3.0;
  double z_real = 3.0;
  double z_imag = 4.0;
  std::string output_dir = ".";
  std::string observable_name = "observable.qasm";
  std::string qasm_name = "qasimfy.qasm";
  bool use_real_space = true;
  std::string layout_preset = "square";
  bool show_help = false;
  std::optional<double> precision;
};

void print_usage(std::string_view prog) {
  std::cout << "Usage: " << prog
            << " [--ell N] [--t value] [--U value] [--E0 value]\n"
            << "             [--z-real value] [--z-imag value]\n"
            << "             [--output-dir path] [--observable-name name]\n"
            << "             [--qasm-name name]\n"
            << "             [--mode real|momentum] [--layout square]\n"
            << "             [--prec value]\n"
            << "             [--help]\n";
}

unsigned deduce_ell_from_L(unsigned L) {
  if (L == 0 || (L & (L - 1)) != 0) {
    throw std::invalid_argument("L must be a positive power of two");
  }
  unsigned ell = 0;
  while ((1u << ell) < L) {
    ++ell;
  }
  return ell;
}

CliOptions parse_cli(int argc, char **argv) {
  CliOptions opts;
  auto expect_value = [&](int &index, const char *name) -> std::string {
    if (index + 1 >= argc) {
      throw std::invalid_argument(std::string("Missing value for ") + name);
    }
    return std::string(argv[++index]);
  };

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help") {
      opts.show_help = true;
    } else if (arg == "--ell") {
      opts.ell =
          static_cast<unsigned>(std::stoul(expect_value(i, arg.c_str())));
    } else if (arg == "--L") {
      unsigned L =
          static_cast<unsigned>(std::stoul(expect_value(i, arg.c_str())));
      opts.ell = deduce_ell_from_L(L);
    } else if (arg == "--t") {
      opts.t = std::stod(expect_value(i, arg.c_str()));
    } else if (arg == "--U") {
      opts.U = std::stod(expect_value(i, arg.c_str()));
    } else if (arg == "--E0") {
      opts.E0 = std::stod(expect_value(i, arg.c_str()));
    } else if (arg == "--z-real") {
      opts.z_real = std::stod(expect_value(i, arg.c_str()));
    } else if (arg == "--z-imag") {
      opts.z_imag = std::stod(expect_value(i, arg.c_str()));
    } else if (arg == "--output-dir") {
      opts.output_dir = expect_value(i, arg.c_str());
    } else if (arg == "--observable-output" || arg == "--observable-name") {
      opts.observable_name = expect_value(i, arg.c_str());
    } else if (arg == "--qasm-output" || arg == "--qasm-name") {
      opts.qasm_name = expect_value(i, arg.c_str());
    } else if (arg == "--mode") {
      auto mode = expect_value(i, arg.c_str());
      if (mode == "real") {
        opts.use_real_space = true;
      } else if (mode == "momentum") {
        opts.use_real_space = false;
      } else {
        throw std::invalid_argument("Unknown mode: " + mode);
      }
    } else if (arg == "--layout") {
      opts.layout_preset = expect_value(i, arg.c_str());
    } else if (arg == "--prec") {
      opts.precision = std::stod(expect_value(i, arg.c_str()));
    } else {
      throw std::invalid_argument("Unknown argument: " + arg);
    }
  }
  return opts;
}

} // namespace

int main(int argc, char **argv) {
  CliOptions cli_opts;
  try {
    cli_opts = parse_cli(argc, argv);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    print_usage(argv[0]);
    return 1;
  }
  if (cli_opts.show_help) {
    print_usage(argv[0]);
    return 0;
  }

  if (cli_opts.layout_preset != "square") {
    std::cerr << "Unsupported layout preset: " << cli_opts.layout_preset
              << std::endl;
    print_usage(argv[0]);
    return 1;
  }

  hubbard::ModelParams params(cli_opts.ell, cli_opts.t, cli_opts.U, cli_opts.E0,
                              {cli_opts.z_real, cli_opts.z_imag});
  hubbard::Layout layout(params);
  const unsigned ell = params.ell;
  const double t = params.t;
  const int num_fermions = layout.num_data_qubits();
  square_hubbard_config &hubbard_config = layout.config();
  const std::complex<double> z = params.z;
  const double E0 = params.E0;

  // Boiler plate
  using namespace tools_v1::ast;
  tools_v1::parser::Position pos;
  std::list<ptr<Stmt>> body;
  auto qreg = RegisterDecl::create(pos, "q", true, num_fermions);
  auto prog = Program::create(pos, true, std::move(body), 0, 0);

  auto data = layout.data_register("q");

  // Create centralized ancilla memory
  tools_v1::tools::ANC_MEM anc_mem;
  hubbard::BuildContext build_ctx{pos, data, anc_mem};

  // clang-format off
  tools_v1::tools::circuit xyz;
  if (cli_opts.use_real_space) {
    auto ziEA_inv_real_for_combo = hubbard::build_ziEA_inverse_real(hubbard_config, t, data, anc_mem, E0, z);
    auto iUB_real = hubbard::build_iUB_real(hubbard_config, data, anc_mem);
    auto I_plus_real = hubbard::build_I_ziEA_inv_iUB(data, std::move(ziEA_inv_real_for_combo), std::move(iUB_real), anc_mem);
    auto AinvB_inv_real = hubbard::build_AinvB_inverse(std::move(I_plus_real), anc_mem);
    auto ziEA_inv_real = hubbard::build_ziEA_inverse_real(hubbard_config, t, data, anc_mem, E0, z);
    xyz = hubbard::build_observable(2, 3, std::move(AinvB_inv_real), std::move(ziEA_inv_real), build_ctx);
  } else {
    auto ziEA_inv_for_combo = hubbard::build_ziEA_inverse(hubbard_config, t, data, anc_mem, num_fermions, ell, E0, z);
    auto iUB = hubbard::build_iUB(hubbard_config, data, anc_mem, num_fermions);
    auto I_plus = hubbard::build_I_ziEA_inv_iUB(data, std::move(ziEA_inv_for_combo), std::move(iUB), anc_mem);
    auto AinvB_inv = hubbard::build_AinvB_inverse(std::move(I_plus), anc_mem);
    auto ziEA_inv = hubbard::build_ziEA_inverse(hubbard_config, t, data, anc_mem, num_fermions, ell, E0, z);
    xyz = hubbard::build_observable(2, 3, std::move(AinvB_inv), std::move(ziEA_inv), build_ctx);
  }
  // clang-format on

  hubbard::materialize_registers(*prog, anc_mem, "q", num_fermions);

  hubbard::push_circuit(prog, xyz);

  std::filesystem::path out_dir = cli_opts.output_dir;
  auto observable_path = (out_dir / cli_opts.observable_name).string();
  auto qasm_path = (out_dir / cli_opts.qasm_name).string();

  std::cout << "Saving program to " << observable_path << std::endl;
  hubbard::save_program(*prog, observable_path);

  std::cout << "QASMifying" << std::endl;
  auto qasm_artifacts = hubbard::qasmify_program(*prog);

  std::cout << "Saving program to " << qasm_path << std::endl;
  hubbard::save_qasm(qasm_path, qasm_artifacts.code);

  if (qasm_artifacts.program) {
    auto resources =
        hubbard::estimate_resources(*qasm_artifacts.program, cli_opts.precision);
    for (const auto &[str, val] : resources) {
      std::cout << str << " :: " << val << std::endl;
    }
  }

  return 0;
}
