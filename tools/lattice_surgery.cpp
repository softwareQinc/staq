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

#include "output/lattice_surgery.hpp"
#include "qasmtools/parser/parser.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    std::string output{};
    bool skip_clifford = false;
    bool skip_litinski = false;
    bool skip_reduce = false;

    CLI::App app{"QASM to lattice surgery instruction compiler"};

    app.add_option("-o,--output", output, "Output to a file");
    app.add_flag("-C,--skip-clifford", skip_clifford,
                 "Skip Clifford operations");
    app.add_flag("-L,--skip-litinski", skip_litinski,
                 "Skip Litinski's transform");
    app.add_flag("-R,--skip-reduce", skip_reduce, "Skip reducing transform");

    CLI11_PARSE(app, argc, argv);
    auto program = parse_stdin();
    if (program) {
        if (output.empty()) {
            output::output_lattice_surgery(*program, skip_clifford,
                                           skip_litinski, skip_reduce);
        } else {
            output::write_lattice_surgery(*program, output, skip_clifford,
                                          skip_litinski, skip_reduce);
        }
    } else {
        std::cerr << "Parsing failed\n";
    }
}
