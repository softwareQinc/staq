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

/**
 * \file synthesis/logic_synthesis.hpp
 * \brief Synthesis of classical logic
 */
#pragma once

#define FMT_HEADER_ONLY = true

#include <lorina/lorina.hpp>
#include <mockturtle/mockturtle.hpp>
#include <tweedledum/tweedledum.hpp>
#include <caterpillar/synthesis/lhrs.hpp>
#include <caterpillar/synthesis/strategies/bennett_mapping_strategy.hpp>
#include <caterpillar/synthesis/strategies/eager_mapping_strategy.hpp>

#include <unordered_map>
#include <fstream>

#include "parser/position.hpp"
#include "ast/stmt.hpp"
#include "utils/angle.hpp"

namespace staq {
namespace synthesis {

enum class Format {
    binary_aiger,
    ascii_aiger,
    bench,
    blif,
    pla,
    verilog,
};

std::unordered_map<std::string, Format> ext_to_format({
    {"aig", Format::binary_aiger},
    {"aag", Format::ascii_aiger},
    {"bench", Format::bench},
    {"blif", Format::blif},
    {"pla", Format::pla},
    {"v", Format::verilog},
});

/**
 * \brief Read in a classical logic network
 */
mockturtle::mig_network read_network(const std::string& fname) {
    mockturtle::mig_network mig;

    std::ifstream ifs;
    ifs.open(fname, std::ifstream::in);
    if (!ifs.good()) {
        std::cerr << "Could not open file \"" << fname << "\"" << std::endl;
        return mig;
    }

    const auto i = fname.find_last_of('.');
    if (i == std::string::npos) {
        std::cerr << "No filename extension" << std::endl;
        return mig;
    }

    const auto it = ext_to_format.find(fname.substr(i + 1));
    if (it == ext_to_format.end()) {
        std::cerr << "Unrecognized file format" << std::endl;
        return mig;
    }

    // Read input file into a logic network
    switch (it->second) {
        case Format::binary_aiger:
            lorina::read_aiger(fname, mockturtle::aiger_reader(mig));
            break;
        case Format::ascii_aiger:
            lorina::read_ascii_aiger(fname, mockturtle::aiger_reader(mig));
            break;
        case Format::verilog:
            lorina::read_verilog(fname, mockturtle::verilog_reader(mig));
            break;
        default:
            std::cerr << "File type not currently cupported" << std::endl;
    }

    return mig;
}

/** \brief Wrapper around ast::angle_to_expr to convert tweedledum angles to
 * ours */
ast::ptr<ast::Expr> angle_to_expr(parser::Position pos,
                                  tweedledum::angle angle) {
    if (angle.is_numerically_defined())
        return ast::angle_to_expr(utils::Angle(angle.numeric_value()));
    else
        return ast::angle_to_expr(utils::Angle(*(angle.symbolic_value())));
}

/**
 * \brief LUT-based hierarchical logic synthesis (arXiv:1706.02721)
 * \note Based on an example given in the caterpillar synthesis library
 */
template <typename T>
std::list<ast::ptr<ast::Gate>>
synthesize_net(parser::Position pos, T& l_net,
               const std::vector<ast::symbol>& params,
               std::string anc = "anc") {
    std::list<ast::ptr<ast::Gate>> ret;

    // Map network into lut with "cut size" 4
    mockturtle::mapping_view<T, true> mapped_network{l_net};
    mockturtle::lut_mapping_params ps;
    ps.cut_enumeration_ps.cut_size = 3;
    mockturtle::lut_mapping<mockturtle::mapping_view<T, true>, true>(
        mapped_network, ps);

    // Collapse network into a klut network
    auto lutn = mockturtle::collapse_mapped_network<mockturtle::klut_network>(
        mapped_network);
    tweedledum::gg_network<tweedledum::mcmt_gate> q_net;
    if (!lutn) {
        std::cerr << "Could not map network into klut network" << std::endl;
        return std::move(ret);
    }

    // Synthesize a gate graph network with 1, 2, and 3 qubit gates using
    // hierarchical synthesis and spectral analysis for klut synthesis.
    // Mapping strategy is eager.

    auto strategy =
        caterpillar::eager_mapping_strategy<mockturtle::klut_network>();
    caterpillar::logic_network_synthesis_params p;
    caterpillar::logic_network_synthesis_stats stats;
    caterpillar::logic_network_synthesis(
        q_net, *lutn, strategy, tweedledum::stg_from_pkrm(), p, &stats);

    // Decompose Toffolis in terms of at most 3-control Toffolis
    q_net = tweedledum::barenco_decomposition(q_net, {3});
    // Decompose further into Clifford + T
    q_net = tweedledum::dt_decomposition(q_net);

    /* AST building */

    // Allocate ancillas
    int num_qubits = q_net.num_qubits();
    int num_inputs =
        static_cast<int>(stats.i_indexes.size() + stats.o_indexes.size());

    if (num_qubits - num_inputs > 0)
        ret.emplace_back(std::make_unique<ast::AncillaDecl>(
            ast::AncillaDecl(pos, anc, false, num_qubits - num_inputs)));

    // Create a mapping from qubits to variable accesses
    auto inputs = stats.i_indexes;
    inputs.insert(inputs.end(), stats.o_indexes.begin(), stats.o_indexes.end());
    if (params.size() != inputs.size()) {
        std::cerr << "Error: expected .v file with " << params.size()
                  << " inputs, got " << inputs.size() << "\n";
        throw ast::SemanticError();
    }

    std::vector<ast::VarAccess> id_refs(num_qubits, ast::VarAccess(pos, anc));

    // Map each input to the correct parameter
    for (int i = 0; i < num_inputs; i++) {
        id_refs[inputs[i]] = ast::VarAccess(pos, params[i]);
    }

    // Map each non-input to an ancilla
    for (int i = 0, cur_anc = 0; i < num_qubits; i++) {
        if (std::find(inputs.begin(), inputs.end(), i) == inputs.end())
            id_refs[i] = ast::VarAccess(pos, anc, cur_anc++);
    }

    // Convert each gate to an ast::Gate
    q_net.foreach_gate([&ret, &id_refs, &pos](auto const& node) {
        auto const& gate = node.gate;
        switch (gate.operation()) {
            case tweedledum::gate_lib::hadamard:
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "h", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::rotation_x: {
                std::vector<ast::ptr<ast::Expr>> args;
                args.emplace_back(angle_to_expr(pos, gate.rotation_angle()));

                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "rx", std::move(args), {id_refs[gate.target()]})));
                break;
            }

            case tweedledum::gate_lib::rotation_y: {
                std::vector<ast::ptr<ast::Expr>> args;
                args.emplace_back(angle_to_expr(pos, gate.rotation_angle()));

                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "ry", std::move(args), {id_refs[gate.target()]})));
                break;
            }

            case tweedledum::gate_lib::rotation_z: {
                std::vector<ast::ptr<ast::Expr>> args;
                args.emplace_back(angle_to_expr(pos, gate.rotation_angle()));

                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "rz", std::move(args), {id_refs[gate.target()]})));
                break;
            }

            case tweedledum::gate_lib::pauli_x:
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "x", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::pauli_y:
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "y", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::t:
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "t", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::phase:
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "s", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::pauli_z:
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "z", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::phase_dagger:
                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "sdg", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::t_dagger:
                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "tdg", {}, {id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::cx:
                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "cx", {},
                        {id_refs[gate.control()], id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::cz:
                ret.emplace_back(
                    std::make_unique<ast::DeclaredGate>(ast::DeclaredGate(
                        pos, "cz", {},
                        {id_refs[gate.control()], id_refs[gate.target()]})));
                break;

            case tweedledum::gate_lib::mcx: {
                // Must have at most 2 controls (i.e. Toffoli gate)
                if (gate.num_controls() > 2) {
                    std::cerr << "Error: too many controls for mcx ("
                              << gate.num_controls() << ")\n";
                    break;
                }

                std::vector<ast::VarAccess> tmp;
                gate.foreach_control([&tmp, &id_refs](auto const& qubit) {
                    tmp.push_back(id_refs[qubit]);
                });
                ret.emplace_back(std::make_unique<ast::DeclaredGate>(
                    ast::DeclaredGate(pos, "ccx", {}, std::move(tmp))));
                break;
            }

            case tweedledum::gate_lib::u3:
            case tweedledum::gate_lib::mcz:
            case tweedledum::gate_lib::swap:
            default:
                std::cerr << "Error: unsupported gate\n";
                break;
        }
    });

    return std::move(ret);
}

} // namespace synthesis
} // namespace staq
