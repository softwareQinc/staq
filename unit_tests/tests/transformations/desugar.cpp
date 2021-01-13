#include "gtest/gtest.h"
#include "parser/parser.hpp"
#include "transformations/desugar.hpp"
#include "transformations/barrier_merge.hpp"

using namespace staq;

// Testing desugaring of mapped gates
/******************************************************************************/
TEST(Desugar, One_Qubit) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "U(0,0,0) q;\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "U(0,0,0) q[0];\n"
                       "U(0,0,0) q[1];\n";

    auto program = parser::parse_string(pre, "one_qubit.qasm");
    transformations::desugar(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Desugar, Two_Qubit) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "qreg p[2];\n"
                      "CX q,p;\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "qreg p[2];\n"
                       "CX q[0],p[0];\n"
                       "CX q[1],p[1];\n";

    auto program = parser::parse_string(pre, "two_qubit.qasm");
    transformations::desugar(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Desugar, Multi_Qubit) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "qreg p[2];\n"
                      "qreg r[2];\n"
                      "barrier q,p,r;\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "qreg p[2];\n"
                       "qreg r[2];\n"
                       "barrier q[0],p[0],r[0],q[1],p[1],r[1];\n";

    auto program = parser::parse_string(pre, "multi_qubit.qasm");
    transformations::desugar(*program);
    transformations::merge_barriers(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Desugar, Mixin) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "qreg p[2];\n"
                      "qreg r[2];\n"
                      "barrier q,p[1],r;\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "qreg p[2];\n"
                       "qreg r[2];\n"
                       "barrier q[0],p[1],r[0],q[1],r[1];\n";

    auto program = parser::parse_string(pre, "mixin.qasm");
    transformations::desugar(*program);
    transformations::merge_barriers(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
