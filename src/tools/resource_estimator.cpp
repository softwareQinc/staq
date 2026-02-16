/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2025 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <cctype>
#include <complex>
#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <third_party/CLI/CLI.hpp>

#include "qasmtools/parser/parser.hpp"

#include "staq/tools/resource_estimator.hpp"

#include <hubbard/builders.hpp>
#include <hubbard/layout.hpp>
#include <hubbard/model_params.hpp>
#include <hubbard/operators.hpp>
#include <hubbard/program_io.hpp>
#include <square_hubbard_circ.hpp>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/parser/parser.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>

namespace {

struct HubbardCliOptions {
    unsigned ell = 7;
    double t = 1.0;
    double U = 4.0;
    double E0 = 3.0;
    double z_real = 3.0;
    double z_imag = 4.0;
    std::string output_dir = ".";
    std::string observable_name = "observable.qasm";
    std::string qasm_name = "qasmify.qasm";
    bool use_real_space = true;
    std::string layout_preset = "square";
    std::optional<double> precision;
};

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

std::string to_lower_copy(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

int run_hubbard_resource_estimator(const HubbardCliOptions& opts) {
    if (to_lower_copy(opts.layout_preset) != "square") {
        std::cerr << "Unsupported layout preset: " << opts.layout_preset
                  << std::endl;
        return 1;
    }

    hubbard::ModelParams params(opts.ell, opts.t, opts.U, opts.E0,
                                {opts.z_real, opts.z_imag});
    hubbard::Layout layout(params);
    const unsigned ell = params.ell;
    const double t = params.t;
    const int num_fermions = layout.num_data_qubits();
    square_hubbard_config& hubbard_config = layout.config();
    const std::complex<double> z = params.z;
    const double E0 = params.E0;

    tools_v1::parser::Position pos;
    std::list<tools_v1::ast::ptr<tools_v1::ast::Stmt>> body;
    auto prog =
        tools_v1::ast::Program::create(pos, true, std::move(body), 0, 0);

    auto data = layout.data_register("q");

    tools_v1::tools::ANC_MEM anc_mem;
    hubbard::BuildContext build_ctx{pos, data, anc_mem};

    tools_v1::tools::circuit observable;
    if (opts.use_real_space) {
        auto ziEA_inv_real_for_combo = hubbard::build_ziEA_inverse_real(hubbard_config, t, data, anc_mem, E0, z);
        auto iUB_real = hubbard::build_iUB_real(hubbard_config, data, anc_mem);
        auto I_plus_real = hubbard::build_I_ziEA_inv_iUB(data, std::move(ziEA_inv_real_for_combo), std::move(iUB_real), anc_mem);
        auto AinvB_inv_real = hubbard::build_AinvB_inverse(std::move(I_plus_real), anc_mem);
        auto ziEA_inv_real = hubbard::build_ziEA_inverse_real(hubbard_config, t, data, anc_mem, E0, z);
        observable = hubbard::build_observable(2, 3, std::move(AinvB_inv_real), std::move(ziEA_inv_real), build_ctx);
    } else {
        auto ziEA_inv_for_combo = hubbard::build_ziEA_inverse(hubbard_config, t, data, anc_mem, num_fermions, ell, E0, z);
        auto iUB = hubbard::build_iUB(hubbard_config, data, anc_mem, num_fermions);
        auto I_plus = hubbard::build_I_ziEA_inv_iUB(data, std::move(ziEA_inv_for_combo), std::move(iUB), anc_mem);
        auto AinvB_inv = hubbard::build_AinvB_inverse(std::move(I_plus), anc_mem);
        auto ziEA_inv = hubbard::build_ziEA_inverse(hubbard_config, t, data, anc_mem, num_fermions, ell, E0, z);
        observable = hubbard::build_observable(2, 3, std::move(AinvB_inv), std::move(ziEA_inv), build_ctx);
    }

    hubbard::materialize_registers(*prog, anc_mem, "q", num_fermions);

    hubbard::push_circuit(prog, observable);

    std::filesystem::path out_dir(opts.output_dir);
    if (!opts.output_dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(out_dir, ec);
        if (ec) {
            std::cerr << "Failed to create output directory '"
                      << opts.output_dir << "': " << ec.message() << std::endl;
            return 1;
        }
    }

    auto observable_path = (out_dir / opts.observable_name).string();
    auto qasm_path = (out_dir / opts.qasm_name).string();
    auto qasm_artifacts = hubbard::qasmify_program(*prog);
    if (qasm_artifacts.program) {
        auto resources = hubbard::estimate_resources(*qasm_artifacts.program, opts.precision);
        for (const auto& [name, value] : resources) {
            std::cout << name << " :: " << value << std::endl;
        }
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    using namespace staq;
    using namespace qasmtools;

    bool unbox_qelib = false;
    bool box_gates = false;
    bool no_merge_dagger = false;
    bool hubbard = false;
    HubbardCliOptions hubbard_opts;
    std::optional<unsigned> hubbard_L_override;
    std::string hubbard_mode =
        hubbard_opts.use_real_space ? "real" : "momentum";

    CLI::App app{"QASM resource estimator"};

    app.add_flag("--box-gates", box_gates,
                 "Treat gate declarations as atomic gates");
    app.add_flag("--unbox-qelib", unbox_qelib,
                 "Unboxes standard library gates");
    app.add_flag("--no-merge-dagger", no_merge_dagger,
                 "Counts gates and their inverses separately");

    app.add_flag("--hubbard", hubbard, "Hubbard Model Resource Estimator");
    app.add_option("--ell", hubbard_opts.ell,
                   "Log2 of the lattice size for the Hubbard estimator");
    app.add_option("--L", hubbard_L_override,
                   "Lattice size (must be a positive power of two)")
        ->check(CLI::PositiveNumber);
    app.add_option("--t", hubbard_opts.t, "Hopping amplitude t");
    app.add_option("--U", hubbard_opts.U, "On-site interaction strength U");
    app.add_option("--E0", hubbard_opts.E0, "Ground state energy offset E0");
    app.add_option("--ReZ,--z-real", hubbard_opts.z_real,
                   "Real part of the spectral shift z");
    app.add_option("--ImZ,--z-imag", hubbard_opts.z_imag,
                   "Imaginary part of the spectral shift z");
    app.add_option("--output-dir", hubbard_opts.output_dir,
                   "Directory used when emitting Hubbard artifacts");
    app.add_option("--observable-name,--observable-output",
                   hubbard_opts.observable_name,
                   "Filename used when saving the observable program");
    app.add_option("--qasm-name,--qasm-output", hubbard_opts.qasm_name,
                   "Filename used when saving the synthesized QASM");
    app.add_option("--mode", hubbard_mode, "Hubbard mode: 'real' or 'momentum'")
        ->check(CLI::IsMember({"real", "momentum"}, CLI::ignore_case));
    app.add_option("--layout", hubbard_opts.layout_preset,
                   "Layout preset (only 'square' supported)");
    app.add_option("--prec", hubbard_opts.precision,
                   "Optional rotation precision used by the Hubbard estimator");

    CLI11_PARSE(app, argc, argv);

    if (hubbard) {
        if (hubbard_L_override) {
            try {
                hubbard_opts.ell = deduce_ell_from_L(*hubbard_L_override);
            } catch (const std::exception& ex) {
                std::cerr << "Invalid --L value: " << ex.what() << std::endl;
                return 1;
            }
        }
        hubbard_opts.use_real_space =
            (to_lower_copy(hubbard_mode) != "momentum");
        return run_hubbard_resource_estimator(hubbard_opts);
    } else {
        auto program = parser::parse_stdin();
        if (program) {

            std::set<std::string_view> overrides =
                unbox_qelib ? std::set<std::string_view>() : ast::qelib_defs;
            auto count = tools::estimate_resources(
                *program, {!box_gates, !no_merge_dagger, overrides});

            std::cout << "Resources used:\n";
            for (auto& [name, num] : count) {
                std::cout << "  " << name << ": " << num << "\n";
            }
        } else {
            std::cerr << "Parsing failed\n";
        }
    }
}
