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
    mpf_set_default_prec(DEFAULT_GMP_PREC);

    bool check, details, verbose;
    real_t theta;
    real_t eps;
    domega_matrix_table_t s3_table;
    str_t tablefile = "";

    CLI::App app{"Grid Synthesis"};

    app.add_option<real_t,float>("-t, --theta", theta, "Z-rotation angle in units of PI")
       ->required();
    app.add_option<real_t,float>("-p, --precision", eps, "Minimum precision of approximation")
       ->required();
    CLI::Option* read = app.add_option("-r, --read-table", tablefile, "Name of file containing s3 table");
    CLI::Option* write = app.add_option("-w, --write-table", tablefile, "Name of table file to write s3_table to.")
                           ->excludes(read);
    app.add_flag("-c, --check", check, "If set, program will output bool that will be 1 if the op string matches the input operator");
    app.add_flag("-d, --details", details, "If set, program will output the particular value of the approximation including the power of root two in the denominator and the true error");
    app.add_flag("-v, --verbose", verbose, "If set program will include additional output");
    
    CLI11_PARSE(app,argc,argv);

<<<<<<< HEAD
    RzApproximation rz_approx = find_rz_approximation(theta*PI, eps);

    domega_matrix_table s3_table = generate_s3_table();
    str_t op_str = synthesize(rz_approx.matrix(), s3_table);
=======
    if(verbose) cout << "Finding approximation" << endl;
>>>>>>> 79d5716 (Now read/write s3_table from file or generate if file doesn't exist)

    RzApproximation rz_approx = find_rz_approximation(theta*PI, eps);

    if(*read) {
        if(verbose) cout << "Reading s3_table from " << tablefile << endl;
        s3_table = read_s3_table(tablefile);
    } else if(*write) {
        if(verbose) cout << "Generating new table file and writing to " << tablefile << endl; 
        s3_table = generate_s3_table();
        write_s3_table(tablefile, s3_table);
    } else if(ifstream(DEFAULT_TABLE_FILE)){
        if(verbose) cout << "Table file found at default location " << DEFAULT_TABLE_FILE << endl;
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if(verbose) cout << "Failed to find " << DEFAULT_TABLE_FILE 
                         << ". Generating new table file and writing to " 
                         << DEFAULT_TABLE_FILE<< endl; 
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE,s3_table);
    } 
    cout << "here" << endl;
    
    if(not rz_approx.solution_found()) {
      cout << "No approximation found for RzApproximation." << endl; 
      return 1;
    }

    str_t op_str = synthesize(rz_approx.matrix(), s3_table);
    
    if(check) {
      cout << (rz_approx.matrix() == domega_matrix_from_str(full_simplify_str(op_str))) << endl;
    }

    if(details) {
      real_t scale = pow(SQRT2,rz_approx.matrix().k());
      cout << rz_approx.matrix() << endl;
      cout << "u decimal value = " << rz_approx.matrix().u().decimal().real()/scale << endl;
      cout << "t decimal value = " << rz_approx.matrix().t().decimal().real()/scale << endl;
      cout << "error = " << rz_approx.error() << endl;
    }

    for(auto &ch : full_simplify_str(op_str)) {
      cout << ch << " "; 
    }
    cout << endl;

    return 0;
}
