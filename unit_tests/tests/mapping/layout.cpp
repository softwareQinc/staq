#include "gtest/gtest.h"
#include "qasmtools/parser/parser.hpp"
#include "mapping/device.hpp"

#include "mapping/layout/basic.hpp"
#include "mapping/layout/eager.hpp"
#include "mapping/layout/bestfit.hpp"

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

// Basic tests for layout generation
/******************************************************************************/
TEST(Layout, Basic) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg orig[9];\n"
                      "CX orig[5],orig[7];\n"
                      "CX orig[7],orig[3];\n"
                      "CX orig[4],orig[0];\n"
                      "CX orig[2],orig[1];\n"
                      "CX orig[6],orig[8];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[5],q[7];\n"
                       "CX q[7],q[3];\n"
                       "CX q[4],q[0];\n"
                       "CX q[2],q[1];\n"
                       "CX q[6],q[8];\n";

    auto program = parser::parse_string(pre, "layout_basic.qasm");
    auto layout = mapping::compute_basic_layout(test_device, *program);
    mapping::apply_layout(layout, test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Layout, Eager) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg orig[9];\n"
                      "CX orig[5],orig[7];\n"
                      "CX orig[7],orig[3];\n"
                      "CX orig[4],orig[0];\n"
                      "CX orig[2],orig[1];\n"
                      "CX orig[6],orig[8];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[0],q[1];\n"
                       "CX q[1],q[4];\n"
                       "CX q[7],q[6];\n"
                       "CX q[2],q[3];\n"
                       "CX q[5],q[8];\n";

    auto program = parser::parse_string(pre, "layout_eager.qasm");
    auto layout = mapping::compute_eager_layout(test_device, *program);
    mapping::apply_layout(layout, test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

/******************************************************************************/
TEST(Layout, Best_Fit) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg orig[9];\n"
                      "CX orig[2],orig[1];\n"
                      "CX orig[2],orig[1];\n"
                      "CX orig[6],orig[8];\n"
                      "CX orig[7],orig[3];\n"
                      "CX orig[7],orig[3];\n"
                      "CX orig[7],orig[3];\n"
                      "CX orig[5],orig[7];\n"
                      "CX orig[5],orig[7];\n"
                      "CX orig[5],orig[7];\n"
                      "CX orig[5],orig[7];\n"
                      "CX orig[4],orig[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[9];\n"
                       "CX q[7],q[6];\n"
                       "CX q[7],q[6];\n"
                       "CX q[5],q[8];\n"
                       "CX q[1],q[4];\n"
                       "CX q[1],q[4];\n"
                       "CX q[1],q[4];\n"
                       "CX q[0],q[1];\n"
                       "CX q[0],q[1];\n"
                       "CX q[0],q[1];\n"
                       "CX q[0],q[1];\n"
                       "CX q[2],q[3];\n";

    auto program = parser::parse_string(pre, "layout_best_fit.qasm");
    auto layout = mapping::compute_bestfit_layout(test_device, *program);
    mapping::apply_layout(layout, test_device, *program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
