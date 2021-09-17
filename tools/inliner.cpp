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

#include <qasm/parser/parser.hpp>
#include "transformations/inline.hpp"

#include <CLI/CLI.hpp>

using namespace staq;
using namespace qasm;

int main(int argc, char** argv) {
    bool clear_decls = false;
    bool inline_stdlib = false;
    std::string ancilla_name = "anc";

    CLI::App app{"QASM inliner"};

    app.add_flag("--clear-decls", clear_decls, "Remove gate declarations");
    app.add_flag("--inline-stdlib", inline_stdlib,
                 "Inline qelib1.inc declarations as well");
    app.add_option("--ancilla-name", ancilla_name,
                   "Name of the global ancilla register, if applicable");

    CLI11_PARSE(app, argc, argv);

    auto program = parser::parse_stdin();
    if (program) {
        std::set<std::string_view> overrides =
            inline_stdlib ? std::set<std::string_view>()
                          : transformations::default_overrides;
        transformations::inline_ast(*program,
                                    {!clear_decls, overrides, ancilla_name});
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
    }
}
