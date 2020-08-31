/*
 * This file is part of staq.
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

#include "parser/parser.hpp"

#include "transformations/desugar.hpp"
#include "transformations/inline.hpp"
#include "transformations/oracle_synthesizer.hpp"
#include "transformations/barrier_merge.hpp"

#include "optimization/simplify.hpp"
#include "optimization/rotation_folding.hpp"
#include "optimization/cnot_resynthesis.hpp"

#include "mapping/device.hpp"
#include "mapping/layout/basic.hpp"
#include "mapping/layout/eager.hpp"
#include "mapping/layout/bestfit.hpp"
#include "mapping/mapping/swap.hpp"
#include "mapping/mapping/steiner.hpp"

#include "tools/resource_estimator.hpp"

#include "output/projectq.hpp"
#include "output/qsharp.hpp"
#include "output/quil.hpp"
#include "output/cirq.hpp"

using namespace staq;

/**
 * \brief Compiler passes
 */
enum class Pass { desugar, inln, synth, rotfold, cnotsynth, simplify, map };

/**
 * \brief Command-line options
 */
enum class Option {
    no_op,
    i,
    S,
    r,
    c,
    s,
    m,
    O1,
    O2,
    O3,
    d,
    l,
    M,
    o,
    f,
    h,
    no_expand,
    disable_lo
};
std::unordered_map<std::string_view, Option> cli_map{
    {"-i", Option::i},
    {"--inline", Option::i},
    {"-S", Option::S},
    {"--synthesize", Option::S},
    {"-r", Option::r},
    {"--rotation-fold", Option::r},
    {"-c", Option::c},
    {"--cnot-resynth", Option::c},
    {"-s", Option::s},
    {"--simplify", Option::s},
    {"-m", Option::m},
    {"--map-to-device", Option::m},
    {"-O1", Option::O1},
    {"-O2", Option::O2},
    {"-O3", Option::O3},
    {"-d", Option::d},
    {"--device", Option::d},
    {"-l", Option::l},
    {"--layout", Option::l},
    {"-M", Option::M},
    {"--mapping-alg", Option::M},
    {"-o", Option::o},
    {"--output", Option::o},
    {"-f", Option::f},
    {"--format", Option::f},
    {"-h", Option::h},
    {"--help", Option::h},
    {"--no-expand-registers", Option::no_expand},
    {"--disable-layout-optimization", Option::disable_lo}};

enum class Layout { linear, eager, bestfit };
enum class Mapper { swap, steiner };
enum class Format { qasm, quil, projectq, qsharp, cirq, resources };

void print_help() {
    int width = 40;

    std::cout << "staq -- (c) 2019 - 2020 softwareQ Inc.\n";
    std::cout << "Usage: staq [PASSES/OPTIONS] FILE.qasm\n\n";
    std::cout << "Compiler passes:\n";
    std::cout << std::setw(width) << std::left << "-i,--inline"
              << "Inline all gates\n";
    std::cout << std::setw(width) << std::left << "-S,--synthesize"
              << "Synthesize oracles defined by logic files\n";
    std::cout << std::setw(width) << std::left << "-r,--rotation-fold"
              << "Apply a rotation optimization pass\n";
    std::cout << std::setw(width) << std::left << "-c,--cnot-resynth"
              << "Apply a CNOT optimization pass\n";
    std::cout << std::setw(width) << std::left << "-s,--simplify"
              << "Apply a simplification pass\n";
    std::cout << std::setw(width) << std::left << "-m,--map-to-device"
              << "Map the circuit to a physical device\n";
    std::cout << std::setw(width) << std::left << "-O1"
              << "Standard light optimization pass\n";
    std::cout << std::setw(width) << std::left << "-O2"
              << "Standard heavy optimization pass\n";
    std::cout << std::setw(width) << std::left << "-O3"
              << "Non-monotonic optimization pass\n\n";
    std::cout << "Options:\n";
    std::cout << std::setw(width) << std::left << "-o,--output FILE"
              << "Output filename. Otherwise prints to stdout.\n";
    std::cout << std::setw(width) << std::left
              << "-f,--format (qasm|quil|projectq|qsharp|cirq|resources) "
              << "Output format. Default=qasm.\n";
    std::cout << std::setw(width) << std::left
              << "-d,--device (tokyo|agave|aspen-4|singapore|square|fullycon) "
              << "Device for physical mapping. Default=tokyo.\n";
    std::cout << std::setw(width) << std::left
              << "-l,--layout (linear|eager|bestfit)"
              << "Initial device layout algorithm. Default=bestfit.\n";
    std::cout << std::setw(width) << std::left
              << "-M,--mapping-alg (swap|steiner)"
              << "Algorithm to use for mapping CNOT gates. Default=steiner.\n";
    std::cout << std::setw(width) << std::left
              << "--disable_layout_optimization"
              << "Disables an expensive layout optimization pass when using "
                 "the steiner mapper.\n";
    std::cout
        << std::setw(width) << std::left << "--no-expand-registers"
        << "Disables expanding gates applied to registers rather than qubits.\n";
}

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << "staq -- (c) 2019 - 2020 softwareQ Inc.\n";
        std::cout << "Usage: staq [PASSES/OPTIONS] FILE.qasm\n";
        std::cout << "Pass --help for additional help\n";
    }

    std::list<Pass> passes{Pass::desugar};

    mapping::Device dev = mapping::tokyo;
    Layout layout_alg = Layout::bestfit;
    Mapper mapper = Mapper::steiner;
    mapping::layout initial_layout;
    std::string ofile = "";
    std::string mapfile = "layout.txt";
    Format format = Format::qasm;
    bool do_lo = true;
    bool mapped = false;

    for (int i = 1; i < argc; i++) {
        switch (cli_map[std::string_view(argv[i])]) {
            /* Passes */
            case Option::i:
                passes.push_back(Pass::inln);
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
            /* Device configuration */
            case Option::d: {
                std::string_view arg(argv[++i]);
                if (arg == "tokyo")
                    dev = mapping::tokyo;
                else if (arg == "agave")
                    dev = mapping::agave;
                else if (arg == "aspen-4")
                    dev = mapping::aspen4;
                else if (arg == "singapore")
                    dev = mapping::singapore;
                else if (arg == "square")
                    dev = mapping::square_9q;
                else if (arg == "fullycon")
                    dev = mapping::fully_connected(9);
                else
                    std::cout << "Error: unrecognized device \"" << arg
                              << "\"\n";
                break;
            }
            case Option::l: {
                std::string_view arg(argv[++i]);
                if (arg == "linear")
                    layout_alg = Layout::linear;
                else if (arg == "eager")
                    layout_alg = Layout::eager;
                else if (arg == "bestfit")
                    layout_alg = Layout::bestfit;
                else
                    std::cout << "Unrecognized layout algorithm \"" << arg
                              << "\"\n";
                break;
            }
            case Option::M: {
                std::string_view arg(argv[++i]);
                if (arg == "swap")
                    mapper = Mapper::swap;
                else if (arg == "steiner")
                    mapper = Mapper::steiner;
                else
                    std::cout << "Unrecognized mapping algorithm \"" << arg
                              << "\"\n";
                break;
            }
            /* Output */
            case Option::o:
                ofile = std::string(argv[++i]);
                break;
            case Option::f: {
                std::string_view arg(argv[++i]);
                if (arg == "qasm")
                    format = Format::qasm;
                else if (arg == "quil")
                    format = Format::quil;
                else if (arg == "projectq")
                    format = Format::projectq;
                else if (arg == "qsharp")
                    format = Format::qsharp;
                else if (arg == "cirq")
                    format = Format::cirq;
                else if (arg == "resources")
                    format = Format::resources;
                else
                    std::cout << "Unrecognized output format \"" << arg
                              << "\"\n";
                break;
            }
            /* Misc */
            case Option::no_expand:
                passes.pop_front();
                break;
            case Option::disable_lo:
                do_lo = false;
                break;
            /* Help */
            case Option::h:
                print_help();
                break;
            /* Default */
            case Option::no_op:
                std::string_view str(argv[i]);
                if (str.substr(str.find_last_of(".") + 1) == "qasm") {

                    /* Parsing */
                    auto prog = parser::parse_file(argv[i]);
                    if (!prog) {
                        std::cout << "Error: failed to parse \"" << str
                                  << "\"\n";
                        exit(0);
                    }

                    /* Passes */
                    for (auto pass : passes)
                        switch (pass) {
                            case Pass::desugar:
                                transformations::desugar(*prog);
                                transformations::merge_barriers(*prog);
                                break;
                            case Pass::inln:
                                transformations::inline_ast(
                                    *prog,
                                    {false, transformations::default_overrides,
                                     "anc"});
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
                                optimization::simplify(*prog);
                                break;
                            case Pass::map: {
                                mapped = true;

                                /* Inline fully first */
                                transformations::inline_ast(*prog,
                                                            {false, {}, "anc"});

                                /* Generate the layout */
                                switch (layout_alg) {
                                    case Layout::linear:
                                        initial_layout =
                                            mapping::compute_basic_layout(
                                                dev, *prog);
                                        break;
                                    case Layout::eager:
                                        initial_layout =
                                            mapping::compute_eager_layout(
                                                dev, *prog);
                                        break;
                                    case Layout::bestfit:
                                        initial_layout =
                                            mapping::compute_bestfit_layout(
                                                dev, *prog);
                                        break;
                                }

                                /* (Optional) optimize the layout */
                                if (mapper == Mapper::steiner && do_lo)
                                    optimize_steiner_layout(dev, initial_layout,
                                                            *prog);

                                /* Apply the layout */
                                mapping::apply_layout(initial_layout, dev,
                                                      *prog);

                                /* Apply the mapping algorithm */
                                switch (mapper) {
                                    case Mapper::swap:
                                        mapping::map_onto_device(dev, *prog);
                                        break;
                                    case Mapper::steiner:
                                        mapping::steiner_mapping(dev, *prog);
                                        break;
                                }

                            }
                        }

                    /* Output */
                    switch (format) {
                        case Format::quil:
                            if (ofile == "")
                                output::output_quil(*prog);
                            else
                                output::write_quil(*prog, ofile);
                            break;
                        case Format::projectq:
                            if (ofile == "")
                                output::output_projectq(*prog);
                            else
                                output::write_projectq(*prog, ofile);
                            break;
                        case Format::qsharp:
                            if (ofile == "")
                                output::output_qsharp(*prog);
                            else
                                output::write_qsharp(*prog, ofile);
                            break;
                        case Format::cirq:
                            if (ofile == "")
                                output::output_cirq(*prog);
                            else
                                output::write_cirq(*prog, ofile);
                            break;
                        case Format::resources: {
                            auto count = tools::estimate_resources(*prog);

                            if (ofile == "") {
                                std::cout << "Resource estimates for " << str
                                          << ":\n";
                                for (auto& [name, num] : count)
                                    std::cout << "  " << name << ": " << num
                                              << "\n";
                            } else {
                                std::ofstream os;
                                os.open(ofile);

                                os << "Resource estimates for " << str << ":\n";
                                for (auto& [name, num] : count)
                                    os << "  " << name << ": " << num << "\n";

                                os.close();
                            }

                            break;
                        }
                        case Format::qasm:
                        default:
                            if (ofile == "") {
                                if (mapped)
                                  dev.print_layout(initial_layout, std::cout, "// ");
                                std::cout << *prog << "\n";
                            } else {
                                std::ofstream os;
                                os.open(ofile);

                                if (mapped)
                                  dev.print_layout(initial_layout, os, "// ");
                                os << *prog;

                                os.close();
                            }
                    }
                } else {
                    std::cout << "Unrecognized option \"" << str << "\"\n";
                    print_help();
                }
                break;
        }
    }
}
