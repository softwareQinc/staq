#include "gtest/gtest.h"

#include "qasmtools/parser/parser.hpp"

#include "staq/optimization/rotation_folding.hpp"

using namespace staq;
using namespace qasmtools;

// Testing rotation folding optimization

TEST(Rotation_folding, T_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "t q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "s q[0];\n";

    auto program = parser::parse_string(pre, "t_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, T_Cancel) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "tdg q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n";

    auto program = parser::parse_string(pre, "t_cancel.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, T_No_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "x q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "t q[0];\n"
                       "x q[0];\n";

    auto program = parser::parse_string(pre, "t_no_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, T_Conj_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "h q[0];\nt q[0];\nh q[0];\n"
                      "x q[0];\n"
                      "h q[0];\nt q[0];\nh q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "h q[0];\nh q[0];\n"
                       "x q[0];\n"
                       "h q[0];\ns q[0];\nh q[0];\n";

    auto program = parser::parse_string(pre, "t_conj_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, Rz_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "rz(pi/16) q[0];\n"
                      "rz(pi/16) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "rz(0.392699081698724) q[0];\n"; // pi/8 to 15 decimals

    auto program = parser::parse_string(pre, "rz_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, Rx_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "rx(pi/16) q[0];\n"
                      "rx(pi/16) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "rx(0.392699081698724) q[0];\n"; // pi/8 to 15 decimals

    auto program = parser::parse_string(pre, "rx_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, T_CNOT_Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[0],q[1];\nt q[1];\ncx q[0],q[1];\n"
                      "cx q[1],q[0];\nt q[0];\ncx q[1],q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "cx q[0],q[1];\ncx q[0],q[1];\n"
                       "cx q[1],q[0];\ns q[0];\ncx q[1],q[0];\n";

    auto program = parser::parse_string(pre, "t_cnot_merge.qasm");
    optimization::fold_rotations(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Rotation_folding, Global_Phase) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "t q[0];\n"
                      "x q[0];\n"
                      "tdg q[0];\n"
                      "x q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[1];\n"
                       "x q[0];\n"
                       "sdg q[0];\n"
                       "x q[0];\n";

    auto program = parser::parse_string(pre, "global_phase.qasm");
    optimization::fold_rotations(*program, {false});
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
