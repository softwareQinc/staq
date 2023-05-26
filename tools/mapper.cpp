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
#include "tools/qubit_estimator.hpp"
#include "transformations/expression_simplifier.hpp"
#include "transformations/inline.hpp"

#include "mapping/device.hpp"
#include "mapping/layout/basic.hpp"
#include "mapping/layout/bestfit.hpp"
#include "mapping/layout/eager.hpp"
#include "mapping/mapping/steiner.hpp"
#include "mapping/mapping/swap.hpp"

// TODO: Find or create a format for reading machine definitions
// and have this tool accept a machine definition as input for mapping

int main(int argc, char** argv) {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    std::string device_json;
    std::string layout = "linear";
    std::string mapper = "swap";
    bool evaluate_all = false;

    CLI::App app{"QASM physical mapper"};
    app.get_formatter()->label("REQUIRED", "(REQUIRED)");
    app.get_formatter()->column_width(40);

    CLI::Option* device_opt =
        app.add_option("-d,--device", device_json, "Device to map onto (.json)")
            ->check(CLI::ExistingFile);
    app.add_option("-l", layout, "Layout algorithm to use. Default=" + layout)
        ->check(CLI::IsMember({"linear", "eager", "bestfit"}));
    app.add_option("-m", mapper, "Mapping algorithm to use. Default=" + mapper)
        ->check(CLI::IsMember({"swap", "steiner"}));
    app.add_flag("--evaluate-all", evaluate_all,
                 "Evaluate all expressions as real numbers");

    CLI11_PARSE(app, argc, argv);

    auto program = parse_stdin();
    if (program) {

        // Inline fully first
        transformations::inline_ast(*program, {false, {}, "anc"});

        // Physical device
        mapping::Device dev;
        if (*device_opt) {
            dev = mapping::parse_json(device_json);
        } else {
            dev = mapping::fully_connected(tools::estimate_qubits(*program));
        }

        // Initial layout
        mapping::layout physical_layout;
        if (layout == "linear") {
            physical_layout = mapping::compute_basic_layout(dev, *program);
        } else if (layout == "eager") {
            physical_layout = mapping::compute_eager_layout(dev, *program);
        } else if (layout == "bestfit") {
            physical_layout = mapping::compute_bestfit_layout(dev, *program);
        }
        mapping::apply_layout(physical_layout, dev, *program);

        // Mapping
        if (mapper == "swap") {
            mapping::map_onto_device(dev, *program);
        } else if (mapper == "steiner") {
            mapping::steiner_mapping(dev, *program);
        }

        /* Evaluating symbolic expressions */
        if (evaluate_all) {
            transformations::expr_simplify(*program, true);
        }

        // Print result
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
