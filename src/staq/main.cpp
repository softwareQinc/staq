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

#include <sstream>

#include <third_party/CLI/CLI.hpp>

#include "qasmtools/parser/parser.hpp"

#include "staq/transformations/barrier_merge.hpp"
#include "staq/transformations/desugar.hpp"
#include "staq/transformations/expression_simplifier.hpp"
#include "staq/transformations/inline.hpp"
#include "staq/transformations/oracle_synthesizer.hpp"

#include "staq/optimization/cnot_resynthesis.hpp"
#include "staq/optimization/rotation_folding.hpp"
#include "staq/optimization/simplify.hpp"

#include "staq/mapping/device.hpp"
#include "staq/mapping/layout/basic.hpp"
#include "staq/mapping/layout/bestfit.hpp"
#include "staq/mapping/layout/eager.hpp"
#include "staq/mapping/mapping/steiner.hpp"
#include "staq/mapping/mapping/swap.hpp"

#include "staq/tools/qubit_estimator.hpp"
#include "staq/tools/resource_estimator.hpp"

#include "staq/output/cirq.hpp"
#include "staq/output/projectq.hpp"
#include "staq/output/qsharp.hpp"
#include "staq/output/quil.hpp"

/**
 * \brief Compiler passes
 */
enum class Pass {
    desugar,
    inln,
    synth,
    rotfold,
    cnotsynth,
    simplify,
    map,
    rewrite
};

/**
 * \brief Command-line passes
 */
enum class Option { none, i, S, r, c, s, m, O1, O2, O3 };
std::unordered_map<std::string_view, Option> cli_map{
    {"-i", Option::i},   {"--inline", Option::i},
    {"-S", Option::S},   {"--synthesize", Option::S},
    {"-r", Option::r},   {"--rotation-fold", Option::r},
    {"-c", Option::c},   {"--cnot-resynth", Option::c},
    {"-s", Option::s},   {"--simplify", Option::s},
    {"-m", Option::m},   {"--map-to-device", Option::m},
    {"-O1", Option::O1}, {"-O2", Option::O2},
    {"-O3", Option::O3}};

/* Passes aren't handled by CLI, so manually create help info */
std::string make_passes_str(int width) {
    std::ostringstream passes_str;
    passes_str << "Compiler passes:\n";
    passes_str << std::setw(width) << std::left << "  -i,--inline"
               << "Inline all gates\n";
    passes_str << std::setw(width) << std::left << "  -S,--synthesize"
               << "Synthesize oracles defined by logic files\n";
    passes_str << std::setw(width) << std::left << "  -r,--rotation-fold"
               << "Apply a rotation optimization pass\n";
    passes_str << std::setw(width) << std::left << "  -c,--cnot-resynth"
               << "Apply a CNOT optimization pass\n";
    passes_str << std::setw(width) << std::left << "  -s,--simplify"
               << "Apply a simplification pass\n";
    passes_str << std::setw(width) << std::left << "  -m,--map-to-device"
               << "Map the circuit to a physical device\n";
    passes_str << std::setw(width) << std::left << "  -O1"
               << "Standard light optimization pass\n";
    passes_str << std::setw(width) << std::left << "  -O2"
               << "Standard heavy optimization pass\n";
    passes_str << std::setw(width) << std::left << "  -O3"
               << "Non-monotonic optimization pass";
    return passes_str.str();
}

int main(int argc, char** argv) {
    using namespace staq;
    using qasmtools::parser::parse_file;

    if (argc == 1) {
        std::cout << "Usage: staq [PASSES/OPTIONS] FILE.qasm\n"
                  << "Run with --help for more information.\n";
        return 0;
    }

    std::string ofile{};
    std::string format = "qasm";
    std::string layout_alg = "bestfit";
    std::string mapper = "steiner";
    bool disable_layout_optimization = false;
    bool no_expand_registers = false;
    bool no_rewrite_expressions = false;
    bool evaluate_all = false;
    std::string device_json;
    std::string input_qasm;

    const std::string copyright_notice{
        "(c) 2019 - 2025 softwareQ Inc. All rights reserved."};
    CLI::App app{"staq -- A full-stack quantum processing toolkit\n" +
                 copyright_notice};
    app.get_formatter()->label("OPTIONS", "PASSES/OPTIONS");
    app.get_formatter()->label("REQUIRED", "(REQUIRED)");
    int width = 43;
    app.get_formatter()->column_width(width);
    app.footer(make_passes_str(width));
    app.allow_extras(); // later call app.remaining() for pass options

    app.add_flag_function(
        "-v,--version",
        [&copyright_notice](auto) {
            std::cout << "staq version " << STAQ_VERSION_STR << "\n";
            std::cout << copyright_notice << "\n";
            exit(EXIT_SUCCESS);
        },
        "Print version information");
    app.add_option("-o,--output", ofile,
                   "Output filename. Otherwise prints to stdout");
    app.add_option("-f,--format", format, "Output format. Default=" + format)
        ->check(CLI::IsMember(
            {"qasm", "quil", "projectq", "qsharp", "cirq", "resources"}));
    app.add_option("-l,--layout", layout_alg,
                   "Initial device layout algorithm. Default=" + layout_alg)
        ->check(CLI::IsMember({"linear", "eager", "bestfit"}));
    app.add_option("-M,--mapping-alg", mapper,
                   "Algorithm to use for mapping CNOT gates. Default=" + mapper)
        ->check(CLI::IsMember({"swap", "steiner"}));
    app.add_flag(
        "--disable-layout-optimization", disable_layout_optimization,
        "Disables an expensive layout optimization pass when using the "
        "steiner mapper");
    app.add_flag(
        "--no-expand-registers", no_expand_registers,
        "Disables expanding gates applied to registers rather than qubits");
    app.add_flag("--no-rewrite-expressions", no_rewrite_expressions,
                 "Disables evaluation of parameter expressions");
    app.add_flag("--evaluate-all", evaluate_all,
                 "Evaluate all expressions as real numbers");
    CLI::Option* device_opt =
        app.add_option("-d,--device", device_json, "Device to map onto (.json)")
            ->check(CLI::ExistingFile);
    app.add_option("FILE.qasm", input_qasm, "OpenQASM circuit")
        ->required()
        ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    /* Passes */
    std::list<Pass> passes;
    if (!no_rewrite_expressions) {
        passes.push_back(Pass::rewrite);
    }
    if (!no_expand_registers) {
        passes.push_back(Pass::desugar);
    }
    for (auto& x : app.remaining()) {
        switch (cli_map[x]) {
            case Option::i:
                passes.push_back(Pass::inln);
                if (!no_rewrite_expressions) {
                    passes.push_back(Pass::rewrite);
                }
                break;
            case Option::S:
                passes.push_back(Pass::synth);
                break;
            case Option::r:
                passes.push_back(Pass::rotfold);
                break;
            case Option::c:
                passes.push_back(Pass::cnotsynth);
                break;
            case Option::s:
                passes.push_back(Pass::simplify);
                break;
            case Option::m:
                passes.push_back(Pass::map);
                break;
            case Option::O1:
                passes.push_back(Pass::rotfold);
                passes.push_back(Pass::simplify);
                break;
            case Option::O2:
                passes.push_back(Pass::inln);
                passes.push_back(Pass::simplify);
                passes.push_back(Pass::rotfold);
                passes.push_back(Pass::simplify);
                break;
            case Option::O3:
                passes.push_back(Pass::inln);
                passes.push_back(Pass::simplify);
                passes.push_back(Pass::rotfold);
                passes.push_back(Pass::simplify);
                passes.push_back(Pass::cnotsynth);
                passes.push_back(Pass::simplify);
                break;
            /* Default */
            case Option::none:
                std::cerr << "Unrecognized option \"" << x << "\"\n";
        }
    }

    mapping::layout initial_layout;
    std::optional<std::map<int, int>> output_perm = std::nullopt;
    bool do_lo = !disable_layout_optimization;
    bool mapped = false;

    /* Deserialization */
    mapping::Device dev;
    if (*device_opt) {
        dev = mapping::parse_json(device_json);
    }

    /* Parsing */
    auto prog = parse_file(input_qasm);
    if (!prog) {
        std::cerr << "Error: failed to parse \"" << input_qasm << "\"\n";
        return 0;
    }

    /* Passes */
    for (auto pass : passes) {
        switch (pass) {
            case Pass::desugar:
                transformations::desugar(*prog);
                transformations::merge_barriers(*prog);
                break;
            case Pass::inln:
                transformations::inline_ast(
                    *prog, {true, transformations::default_overrides, "anc"});
                break;
            case Pass::synth:
                transformations::synthesize_oracles(*prog);
                break;
            case Pass::rotfold:
                optimization::fold_rotations(*prog);
                break;
            case Pass::cnotsynth:
                optimization::optimize_CNOT(*prog);
                break;
            case Pass::simplify:
                transformations::expr_simplify(*prog);
                optimization::simplify(*prog);
                break;
            case Pass::map: {
                mapped = true;

                /* Inline fully first */
                transformations::inline_ast(*prog, {true, {}, "anc"});

                /* Device */
                if (!(*device_opt)) {
                    dev =
                        mapping::fully_connected(tools::estimate_qubits(*prog));
                }

                /* Generate the layout */
                if (layout_alg == "linear") {
                    initial_layout = mapping::compute_basic_layout(dev, *prog);
                } else if (layout_alg == "eager") {
                    initial_layout = mapping::compute_eager_layout(dev, *prog);
                } else if (layout_alg == "bestfit") {
                    initial_layout =
                        mapping::compute_bestfit_layout(dev, *prog);
                }

                /* (Optional) optimize the layout */
                if (mapper == "steiner" && do_lo) {
                    optimize_steiner_layout(dev, initial_layout, *prog);
                }

                /* Apply the layout */
                mapping::apply_layout(initial_layout, dev, *prog);

                /* Apply the mapping algorithm */
                if (mapper == "swap") {
                    output_perm = mapping::map_onto_device(dev, *prog);
                } else if (mapper == "steiner") {
                    mapping::steiner_mapping(dev, *prog);
                }
                break;
            }
            case Pass::rewrite:
                transformations::expr_simplify(*prog, evaluate_all);
                break;
        }
    }

    /* Evaluating symbolic expressions */
    if (evaluate_all) {
        transformations::expr_simplify(*prog, true);
    }

    /* Output */
    if (format == "quil") {
        if (ofile.empty()) {
            output::output_quil(*prog);
        } else {
            output::write_quil(*prog, ofile);
        }
    } else if (format == "projectq") {
        if (ofile.empty()) {
            output::output_projectq(*prog);
        } else {
            output::write_projectq(*prog, ofile);
        }
    } else if (format == "qsharp") {
        if (ofile.empty()) {
            output::output_qsharp(*prog);
        } else {
            output::write_qsharp(*prog, ofile);
        }
    } else if (format == "cirq") {
        if (ofile.empty()) {
            output::output_cirq(*prog);
        } else {
            output::write_cirq(*prog, ofile);
        }
    } else if (format == "resources") {
        auto count = tools::estimate_resources(*prog);

        if (ofile.empty()) {
            std::cout << "Resource estimates for " << input_qasm << ":\n";
            for (auto& [name, num] : count) {
                std::cout << "  " << name << ": " << num << "\n";
            }
        } else {
            std::ofstream os;
            os.open(ofile);

            os << "Resource estimates for " << input_qasm << ":\n";
            for (auto& [name, num] : count) {
                os << "  " << name << ": " << num << "\n";
            }

            os.close();
        }
    } else { // qasm format
        if (ofile.empty()) {
            if (mapped) {
                dev.print_layout(initial_layout, std::cout, "// ", output_perm);
            }
            std::cout << *prog << "\n";
        } else {
            std::ofstream os;
            os.open(ofile);

            if (mapped) {
                dev.print_layout(initial_layout, os, "// ", output_perm);
            }
            os << *prog;

            os.close();
        }
    }
}
