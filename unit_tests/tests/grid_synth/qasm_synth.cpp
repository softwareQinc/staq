#include "gtest/gtest.h"

#include "qasmtools/parser/parser.hpp"

#include "staq/transformations/qasm_synth.hpp"

using namespace staq;
using namespace grid_synth;
using qasmtools::parser::parse_string;

// Tests rz gate replacement.
// A multiple of pi/4 is used so that the result is deterministic.
TEST(QasmSynth, ExactSynthesis) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "rz(2*pi/4) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "s q[0];\n";

    auto program = parse_string(pre, "exact_synthesis.qasm");
    GridSynthOptions opt{100, 200, false, false, false, false};
    int w_count = transformations::qasm_synth(*program, opt);
    EXPECT_EQ(w_count, 14);

    std::stringstream ss;
    ss << *program;
    EXPECT_EQ(ss.str(), post);
}

// Tests collection of w and W gates into the global phase.
TEST(QasmSynth, GlobalPhase) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "rz(pi/2) q[0];\n"  // phase += 14/8
                      "rz(pi/4) q[0];\n"; // phase -= 1/8

    // 32 of these gates should not modify the global phase
    for (int i = 0; i < 32; ++i) {
        pre += "rz(pi/2) q[0];\n";
    }

    auto program = parse_string(pre, "global_phase.qasm");
    GridSynthOptions opt{100, 200, false, false, false, false};
    int w_count = transformations::qasm_synth(*program, opt);
    EXPECT_EQ(w_count, 13); // phase should be 13/8
}

// Tests rz gate replacement when the exact solution is not known.
// Also tests GMP expression parsing, and angle caching in GridSynthesizer.
TEST(QasmSynth, InexactSynthesis) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "rz(-0.3) q[0];\n"  // These three rz gates all
                      "rz(-3/10) q[0];\n" //  have the same angle
                      "rz(9*-27/100*10/81) q[0];\n";

    auto program = parse_string(pre, "inexact_synthesis.qasm");
    GridSynthOptions opt{5, 200, false, false, false, false};
    transformations::qasm_synth(*program, opt);
    std::stringstream ss;
    ss << *program;

    // Grab the rz gate replacements.
    std::vector<std::string> gates;
    std::string line;
    bool push = false;
    while (getline(ss, line)) {
        if (push) {
            gates.push_back(line);
        } else if (line == "qreg q[2];") {
            push = true;
        }
    }

    // Although rz inexact synthesis is non-deterministic, these rz gates have
    //  the same angle, and thus they should have the same replacement due to
    //  GridSynthesizer angle caching.
    int N = gates.size();
    EXPECT_TRUE(N > 1);
    EXPECT_TRUE(N % 3 == 0);
    for (int i = 0; i < gates.size() / 3; ++i) {
        EXPECT_EQ(gates[i], gates[i + N / 3]);
        EXPECT_EQ(gates[i], gates[i + 2 * N / 3]);
    }
}

// Tests rx gate replacement.
// A multiple of pi/4 is used so that the result is deterministic.
TEST(QasmSynth, rx) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "rx(pi) q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "h q[1];\n"
                       "s q[1];\n"
                       "s q[1];\n"
                       "h q[1];\n";

    auto program = parse_string(pre, "exact_synthesis.qasm");
    GridSynthOptions opt{100, 200, false, false, false, false};
    int w_count = transformations::qasm_synth(*program, opt);
    EXPECT_EQ(w_count, 12);

    std::stringstream ss;
    ss << *program;
    EXPECT_EQ(ss.str(), post);
}

// Tests ry gate replacement.
// A multiple of pi/4 is used so that the result is deterministic.
// Also tests logic for handling common cases in the range [2,4).
TEST(QasmSynth, ry) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "ry(15/2*pi) q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "s q[1];\n"
                       "h q[1];\n"
                       "s q[1];\n"
                       "s q[1];\n"
                       "s q[1];\n"
                       "h q[1];\n"
                       "sdg q[1];\n";

    auto program = parse_string(pre, "exact_synthesis.qasm");
    GridSynthOptions opt{100, 200, false, false, false, false};
    int w_count = transformations::qasm_synth(*program, opt);
    EXPECT_EQ(w_count, 2);

    std::stringstream ss;
    ss << *program;
    EXPECT_EQ(ss.str(), post);
}
