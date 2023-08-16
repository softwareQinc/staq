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
#include <iostream>

#include "grid_synth/types.hpp"
#include "grid_synth/rz_approximation.hpp"
#include "grid_synth/exact_synthesis.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;
    using namespace std;

    bool verify, details;
    real_t theta;
    real_t eps;

    CLI::App app{"Grid Synthesis"};

    app.add_option<real_t,float>("-t, --theta", theta, "Z-rotation angle in units of PI")
       ->required();
    app.add_option<real_t,float>("-p, --precision", eps, "Minimum Precision of Approximation")
       ->required();
    app.add_flag("-v, --verify", verify, "If set, program will output bool that will be 1 if the op string matches the input operator");
    app.add_flag("-d, --details", details, "If set, program will output the particular value of the approximation including the power of root two in the denominator and the true error");

    CLI11_PARSE(app,argc,argv);

    RzApproximation rz_approx = find_rz_approximation(theta*PI, eps);

    domega_matrix_table s3_table = generate_s3_table();
    str_t op_str = synthesize(rz_approx.matrix(), s3_table);

    if(verify) {
      cout << (rz_approx.matrix() == domega_matrix_from_str(op_str)) << endl;
    }

    if(details) {
      real_t scale = pow(SQRT2,rz_approx.matrix().k());
      cout << rz_approx.matrix() << endl;
      cout << "u decimal value = " << rz_approx.matrix().u().decimal().real()/scale << endl;
      cout << "t decimal value = " << rz_approx.matrix().t().decimal().real()/scale << endl;
      cout << "error = " << rz_approx.error() << endl;
    }

    cout << simplify_str(op_str) << endl;

    return 0;
}
