#include <CLI/CLI.hpp>
#include <iostream>

#include "grid_synth/gmp_functions.hpp"
#include "grid_synth/types.hpp"
#include "qasmtools/parser/parser.hpp"
#include "transformations/qasm_synth.hpp"

int main(int argc, char** argv) {
    using namespace staq;
    using namespace grid_synth;
    using qasmtools::parser::parse_stdin;

    bool check = false, details = false, verbose = false;
    real_t eps;
    long int prec;
    int factor_effort;
    domega_matrix_table_t s3_table;
    str_t tablefile{};

    CLI::App app{"rx/ry/rz substitution"};

    // this interface is more or less identical to that of grid_synth.cpp
    // TODO: consider factoring out duplicated code?
    CLI::Option* prec_opt =
        app.add_option<long int, int>(
               "-p, --precision", prec,
               "Precision in base ten as a positive integer (10^-p)")
            ->required();
    CLI::Option* fact_eff = app.add_option<int, int>(
        "--pollard-rho", factor_effort,
        "Sets MAX_ATTEMPTS_POLLARD_RHO, the effort "
        "taken to factorize candidate solutions (default=200)");
    CLI::Option* read = app.add_option("-r, --read-table", tablefile,
                                       "Name of file containing s3 table");
    CLI::Option* write =
        app.add_option("-w, --write-table", tablefile,
                       "Name of table file to write s3_table to")
            ->excludes(read);
    app.add_flag("-c, --check", check,
                 "Output bool that will be 1 if the op string matches the "
                 "input operator");
    app.add_flag(
        "-d, --details", details,
        "Output the particular value of the approximation including the power "
        "of root two in the denominator, the true error, and the T-count.");
    app.add_flag("-v, --verbose", verbose,
                 "Include additional output during runtime such as runtime "
                 "parameters and update on each step.");

    CLI11_PARSE(app, argc, argv);

    if (*read) {
        if (verbose) {
            std::cerr << "Reading s3_table from " << tablefile << '\n';
        }
        s3_table = read_s3_table(tablefile);
    } else if (*write) {
        if (verbose) {
            std::cerr << "Generating new table file and writing to "
                      << tablefile << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(tablefile, s3_table);
    } else if (std::ifstream(DEFAULT_TABLE_FILE)) {
        if (verbose) {
            std::cerr << "Table file found at default location "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = read_s3_table(DEFAULT_TABLE_FILE);
    } else {
        if (verbose) {
            std::cerr << "Failed to find " << DEFAULT_TABLE_FILE
                      << ". Generating new table file and writing to "
                      << DEFAULT_TABLE_FILE << '\n';
        }
        s3_table = generate_s3_table();
        write_s3_table(DEFAULT_TABLE_FILE, s3_table);
    }

    MP_CONSTS = initialize_constants(prec);
    eps = gmpf::pow(real_t(10), -prec);

    if (*fact_eff) {
        MAX_ATTEMPTS_POLLARD_RHO = factor_effort;
    }

    if (verbose) {
        std::cerr << "Runtime Parameters" << '\n';
        std::cerr << "------------------" << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "TOL (Tolerance for float equality) " << std::setw(1)
                  << ": " << std::setw(3 * COLW) << std::left << std::scientific
                  << TOL << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "KMIN (Minimum scaling exponent) " << std::setw(1) << ": "
                  << std::setw(3 * COLW) << std::left << std::fixed << KMIN
                  << '\n';
        std::cerr << std::setw(2 * COLW) << std::left
                  << "KMAX (Maximum scaling exponent) " << std::setw(1) << ": "
                  << std::setw(3 * COLW) << std::left << std::fixed << KMAX
                  << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "MAX_ATTEMPTS_POLLARD_RHO (How hard we try to factor) "
                  << std::setw(1) << ": " << std::setw(3 * COLW) << std::left
                  << MAX_ATTEMPTS_POLLARD_RHO << '\n';
        std::cerr << std::setw(3 * COLW) << std::left
                  << "MAX_ITERATIONS_FERMAT_TEST (How hard we try to check "
                     "primality) "
                  << std::setw(1) << ": " << std::setw(3 * COLW) << std::left
                  << MAX_ITERATIONS_FERMAT_TEST << '\n';
    }
    std::cerr << std::scientific;

    auto program = parse_stdin("", true); // parse stdin using GMP
    if (program) {
        transformations::replace_rz(*program, s3_table, eps, check, details,
                                    verbose);
        std::cout << *program;
    } else {
        std::cerr << "Parsing failed\n";
        return 1;
    }
}
