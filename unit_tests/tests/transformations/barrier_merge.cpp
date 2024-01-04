#include "gtest/gtest.h"

#include "qasmtools/parser/parser.hpp"

#include "staq/transformations/barrier_merge.hpp"

using namespace staq;
using namespace qasmtools;

// Testing merging of adjacent barriers

TEST(BarrierMerge, Adjacent) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "barrier q[0];\n"
                      "barrier q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "barrier q[0],q[1];\n";

    auto program = parser::parse_string(pre, "adjacent.qasm");
    transformations::merge_barriers(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}

// Testing merging of non-adjacent barriers

TEST(BarrierMerge, NonAdjacent) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "barrier q[0];\n"
                      "CX q[0],q[1];\n"
                      "barrier q[1];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "barrier q[0];\n"
                       "CX q[0],q[1];\n"
                       "barrier q[1];\n";

    auto program = parser::parse_string(pre, "nonadjacent.qasm");
    transformations::merge_barriers(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
