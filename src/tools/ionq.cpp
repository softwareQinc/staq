/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2023 softwareQ Inc. All rights reserved.
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

#include <CLI/CLI.hpp>

#include "qasmtools/parser/parser.hpp"

#include "staq/mapping/device.hpp"
#include "staq/mapping/layout/basic.hpp"
#include "staq/output/ionq.hpp"
#include "staq/transformations/desugar.hpp"
#include "staq/transformations/expression_simplifier.hpp"
#include "staq/transformations/inline.hpp"
#include "staq/transformations/replace_ugate.hpp"

static const std::set<std::string_view> ionq_overrides{
    "x",  "y",  "z",  "h",  "s",    "sdg", "t",  "tdg", "rx",
    "ry", "rz", "cz", "cy", "swap", "cx",  "u1", "ch",  "crz"};

int main(int argc, char** argv) {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    std::string filename = "";

    CLI::App app{"QASM to IonQ transpiler"};

    app.add_option("-o,--output", filename, "Output to a file");

    CLI11_PARSE(app, argc, argv);
    auto program = parse_stdin();

    if (program) {
        transformations::desugar(*program);

        // Flatten qregs into one global qreg.
        // IonQ Simulator has 29 qubits;
        //  the other IonQ devices have less than 29 qubits.
        // For now let's set a cap of 11 qubits; change this later.
        auto device = mapping::fully_connected(11);
        auto layout = mapping::compute_basic_layout(device, *program);
        mapping::apply_layout(layout, device, *program);

        // Inline declared gates
        transformations::Inliner::config params{false, ionq_overrides};
        transformations::inline_ast(*program, params);

        // Evaluate expressions
        // TODO: Handle multiples of pi nicely
        transformations::expr_simplify(*program, true);

        // Replace U gates
        transformations::replace_ugates(*program);

        if (filename.empty())
            output::output_ionq(*program);
        else
            output::write_ionq(*program, filename);
    } else {
        std::cerr << "Parsing failed\n";
    }
}
