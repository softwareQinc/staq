#include "gtest/gtest.h"

#include "qasmtools/parser/parser.hpp"

#include "staq/optimization/cnot_resynthesis.hpp"

using namespace staq;
using namespace qasmtools;

// Testing cnot resynthesis

TEST(CNOT_resynthesis, Base) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[1],q[0];\n"
                      "t q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "cx q[1],q[0];\n"
                       "t q[0];\n";

    auto program = parser::parse_string(pre, "base.qasm");
    optimization::optimize_CNOT(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(CNOT_resynthesis, Merge) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "cx q[1],q[0];\n"
                      "t q[0];\n"
                      "t q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "cx q[1],q[0];\n"
                       "s q[0];\n";

    auto program = parser::parse_string(pre, "merge.qasm");
    optimization::optimize_CNOT(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(CNOT_resynthesis, Decl_opt) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "gate foo a,b,c {\n"
                      "\tt c;\n"
                      "\tcx c,b;\n"
                      "\tcx a,b;\n"
                      "\tt b;\n"
                      "\tcx b,a;\n"
                      "\tt a;\n"
                      "\tcx a,c;\n"
                      "\tcx b,c;\n"
                      "\tt c;\n"
                      "\tcx b,a;\n"
                      "\tcx a,c;\n"
                      "\tcx a,b;\n"
                      "\tcx c,b;\n"
                      "}\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "gate foo a,b,c {\n"
                       "\tt c;\n"
                       "\tcx b,c;\n"
                       "\tt c;\n"
                       "\tcx a,c;\n"
                       "\tt c;\n"
                       "\tcx b,c;\n"
                       "\tt c;\n"
                       "\tcx a,c;\n"
                       "}\n";

    auto program = parser::parse_string(pre, "decl_opt.qasm");
    optimization::optimize_CNOT(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
