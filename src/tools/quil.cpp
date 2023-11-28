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

#include <third_party/CLI/CLI.hpp>

#include "output/quil.hpp"
#include "qasmtools/parser/parser.hpp"
#include "transformations/desugar.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using qasmtools::parser::parse_stdin;

    std::string filename = "";

    CLI::App app{"QASM to QUIL transpiler"};

    app.add_option("-o,--output", filename, "Output to a file");

    CLI11_PARSE(app, argc, argv);
    auto program = parse_stdin();
    if (program) {
        transformations::desugar(*program);
        if (filename.empty())
            output::output_quil(*program);
        else
            output::write_quil(*program, filename);
    } else {
        std::cerr << "Parsing failed\n";
    }
}
