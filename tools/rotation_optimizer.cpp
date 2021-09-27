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

#include <qasmtools/parser/parser.hpp>
#include "optimization/rotation_folding.hpp"

#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    bool no_correction = false;

    CLI::App app{"QASM rotation optimizer"};

    app.add_flag("--no-phase-correction", no_correction,
                 "Turns off global phase corrections");

    CLI11_PARSE(app, argc, argv);

    auto program = parse_stdin();
    if (program) {
        optimization::fold_rotations(*program, {!no_correction});
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
