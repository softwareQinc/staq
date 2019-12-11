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

#include "ast/ast.hpp"
#include "parser/parser.hpp"
#include "tools/resource_estimator.hpp"

#include <CLI/CLI.hpp>

using namespace staq;

int main(int argc, char** argv) {
  bool unbox_qelib = false;
  bool box_gates = false;
  bool no_merge_dagger = false;

  CLI::App app{ "QASM resource estimator" };

  app.add_flag("--box-gates", box_gates, "Treat gate declarations as atomic gates");
  app.add_flag("--unbox-qelib", unbox_qelib, "Unboxes standard library gates");
  app.add_flag("--no-merge-dagger", no_merge_dagger, "Counts gates and their inverses separately");

  CLI11_PARSE(app, argc, argv);

  auto program = parser::parse_stdin();
  if (program) {

    std::set<std::string_view> overrides = unbox_qelib ? std::set<std::string_view>() : ast::qelib_defs;
    auto count = tools::estimate_resources(*program, { !box_gates, !no_merge_dagger, overrides });

    std::cout << "Resources used:\n";
    for (auto& [name, num] : count) {
      std::cout << "  " << name << ": " << num << "\n";
    }
  } else {
    std::cerr << "Parsing failed\n";
  }

  return 1;
}
