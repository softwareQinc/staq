#include "gtest/gtest.h"

#include "qasmtools/parser/parser.hpp"

// clang-format off
#include "staq/mapping/device.hpp"
#include "staq/mapping/mapping/swap.hpp"
#include "staq/mapping/mapping/steiner.hpp"
// clang-format on

using namespace staq;
using namespace qasmtools;

static mapping::Device test_device("Test device", 9,
                                   {
                                       {0, 1, 0, 0, 0, 1, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 0, 0, 0},
                                       {0, 1, 0, 1, 0, 0, 0, 0, 0},
                                       {0, 0, 1, 0, 1, 0, 0, 0, 1},
                                       {0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 0, 0, 1, 0, 1, 0, 0},
                                       {0, 0, 0, 0, 0, 1, 0, 1, 0},
                                       {0, 0, 0, 0, 1, 0, 1, 0, 1},
                                       {0, 0, 0, 1, 0, 0, 0, 1, 0},
                                   },
                                   {1, 1, 1, 1, 1, 1, 1, 1, 1},
                                   {
                                       {0, 0.9, 0, 0, 0, 0.1, 0, 0, 0},
                                       {0.1, 0, 0.1, 0, 0.8, 0, 0, 0, 0},
                                       {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
                                       {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
                                       {0, 0.1, 0, 0.1, 0, 0.1, 0, 0.7, 0},
                                       {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
                                       {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
                                       {0, 0, 0, 0, 0.1, 0, 0.6, 0, 0.5},
                                       {0, 0, 0, 0.1, 0, 0, 0, 0.1, 0},
                                   });

// Basic tests for mapping CNOT gates

TEST(Swap_Mapper, Base) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[9];\n"
                      "CX q[0],q[2];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[0],q[1];\n"
                       "CX q[1],q[0];\n"
                       "CX q[0],q[1];\n"
                       "CX q[1],q[2];\n";

    auto program = parser::parse_string(pre, "swap_base.qasm");
    mapping::map_onto_device(test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Swap_Mapper, Shortest_Path) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[9];\n"
                      "CX q[0],q[6];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[0],q[1];\n"
                       "CX q[1],q[0];\n"
                       "CX q[0],q[1];\n"
                       "CX q[1],q[4];\n"
                       "CX q[4],q[1];\n"
                       "CX q[1],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[7],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[7],q[6];\n";

    auto program = parser::parse_string(pre, "swap_shortest_path.qasm");
    mapping::map_onto_device(test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Steiner_Mapper, Base) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[9];\n"
                      "CX q[0],q[2];\n"
                      "CX q[0],q[6];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[1],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[7],q[6];\n"
                       "CX q[1],q[2];\n"
                       "CX q[4],q[7];\n"
                       "CX q[1],q[4];\n"
                       "CX q[0],q[1];\n"
                       "CX q[1],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[7],q[6];\n"
                       "CX q[1],q[2];\n"
                       "CX q[4],q[7];\n"
                       "CX q[1],q[4];\n"
                       "CX q[0],q[1];\n";

    auto program = parser::parse_string(pre, "steiner_base.qasm");
    mapping::steiner_mapping(test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Steiner_Mapper, Swap) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[9];\n"
                      "CX q[7],q[1];\n"
                      "CX q[1],q[7];\n"
                      "CX q[7],q[1];\n"
                      "U(0,0,pi/4) q[1];\n"
                      "CX q[7],q[1];\n"
                      "CX q[1],q[7];\n"
                      "CX q[7],q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "U(0,0,pi/4) q[7];\n";

    auto program = parser::parse_string(pre, "steiner_swap.qasm");
    mapping::steiner_mapping(test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

TEST(Steiner_Mapper, Swap_No_Z) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[9];\n"
                      "CX q[7],q[1];\n"
                      "CX q[1],q[7];\n"
                      "CX q[7],q[1];\n"
                      "U(0,pi/4,0) q[1];\n"
                      "CX q[7],q[1];\n"
                      "CX q[1],q[7];\n"
                      "CX q[7],q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[7],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[4],q[1];\n"
                       "CX q[7],q[4];\n"
                       "CX q[1],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[4],q[1];\n"
                       "CX q[7],q[4];\n"
                       "U(0,pi/4,0) q[1];\n"
                       "CX q[7],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[4],q[1];\n"
                       "CX q[7],q[4];\n"
                       "CX q[1],q[4];\n"
                       "CX q[4],q[7];\n"
                       "CX q[4],q[1];\n"
                       "CX q[7],q[4];\n";

    auto program = parser::parse_string(pre, "steiner_swap_no_z.qasm");
    mapping::steiner_mapping(test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
