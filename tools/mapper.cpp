/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2021 softwareQ Inc. All rights reserved.
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
#include "transformations/inline.hpp"

#include "mapping/device.hpp"
#include "mapping/layout/basic.hpp"
#include "mapping/layout/eager.hpp"
#include "mapping/layout/bestfit.hpp"
#include "mapping/mapping/swap.hpp"
#include "mapping/mapping/steiner.hpp"

#include <CLI/CLI.hpp>

using namespace staq;

// TODO: Find or create a format for reading machine definitions
// and have this tool accept a machine definition as input for mapping

int main(int argc, char** argv) {
    std::string device_name = "tokyo";
    std::string layout = "linear";
    std::string mapper = "swap";

    CLI::App app{"QASM physical mapper"};

    app.add_option(
        "-d", device_name,
        "Device to map onto (tokyo|agave|aspen-4|singapore|square|fullycon)");
    app.add_option("-l", layout,
                   "Layout algorithm to use (linear|eager|bestfit)");
    app.add_option("-m", mapper, "Mapping algorithm to use (swap|steiner)");

    CLI11_PARSE(app, argc, argv);

    auto program = parser::parse_stdin();
    if (program) {

        // Inline fully first
        transformations::inline_ast(*program, {false, {}, "anc"});

        // Physical device
        mapping::Device dev;
        if (device_name == "tokyo") {
            dev = mapping::tokyo;
        } else if (device_name == "agave") {
            dev = mapping::agave;
        } else if (device_name == "aspen-4") {
            dev = mapping::aspen4;
        } else if (device_name == "singapore") {
            dev = mapping::singapore;
        } else if (device_name == "square") {
            dev = mapping::square_9q;
        } else if (device_name == "fullycon") {
            dev = mapping::fully_connected(9);
        } else {
            std::cerr << "Error: invalid device name\n";
            return 0;
        }

        // Initial layout
        mapping::layout physical_layout;
        if (layout == "linear") {
            physical_layout = mapping::compute_basic_layout(dev, *program);
        } else if (layout == "eager") {
            physical_layout = mapping::compute_eager_layout(dev, *program);
        } else if (layout == "bestfit") {
            physical_layout = mapping::compute_bestfit_layout(dev, *program);
        } else {
            std::cerr << "Error: invalid layout algorithm\n";
            return 0;
        }
        mapping::apply_layout(physical_layout, dev, *program);

        // Mapping
        if (mapper == "swap") {
            mapping::map_onto_device(dev, *program);
        } else if (mapper == "steiner") {
            mapping::steiner_mapping(dev, *program);
        } else {
            std::cerr << "Error: invalid mapping algorithm\n";
            return 0;
        }

        // Print result
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
