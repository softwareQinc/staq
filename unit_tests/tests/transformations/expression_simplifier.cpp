#include "gtest/gtest.h"

#include "qasmtools/parser/parser.hpp"

#include "transformations/expression_simplifier.hpp"

using namespace staq;
using namespace qasmtools;

// Testing collecting multiples of pi
/******************************************************************************/
TEST(ExprSimplify, CollectPi) {
    std::string pre =
        "OPENQASM 2.0;\n"
        "include \"qelib1.inc\";\n"
        "\n"
        "qreg q[2];\n"
        "rz((((-(pi/4)/2)+(-(pi/4)/2))+(-(pi/4)/2))+(-(pi/4)/2)) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "qreg q[2];\n"
                       "rz(-pi/2) q[0];\n";

    auto program = parser::parse_string(pre, "collect_pi.qasm");
    transformations::expr_simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

// Testing simplifying expressions with rational numbers
/******************************************************************************/
TEST(ExprSimplify, Rationals) {
    std::string pre = "OPENQASM 2.0;\n"
                      "\n"
                      "qreg q[2];\n"
                      "U(6/15,(1+9)/(3-1/2),-1/2-1/3) q[0];\n";

    std::string post = "OPENQASM 2.0;\n"
                       "\n"
                       "qreg q[2];\n"
                       "U(2/5,4,-5/6) q[0];\n";

    auto program = parser::parse_string(pre, "rationals.qasm");
    transformations::expr_simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

// Testing variable expressions
/******************************************************************************/
TEST(ExprSimplify, Variables) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "gate mygate(lambda) q {\n"
                      "\trz(0.5-1/2+lambda) q;\n"
                      "\trz(0-(lambda+3.5)) q;\n"
                      "\trz(lambda+(3.5-0.5*7)) q;\n"
                      "\trz((lambda+pi)/(2-1)) q;\n"
                      "}\n";

    std::string post = "OPENQASM 2.0;\n"
                       "include \"qelib1.inc\";\n"
                       "\n"
                       "gate mygate(lambda) q {\n"
                       "\trz(lambda) q;\n"
                       "\trz(-(lambda+3.5)) q;\n"
                       "\trz(lambda) q;\n"
                       "\trz(lambda+pi) q;\n"
                       "}\n";

    auto program = parser::parse_string(pre, "variables.qasm");
    transformations::expr_simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/

// Testing complicated expressions
/******************************************************************************/
TEST(ExprSimplify, Mixed) {
    std::string pre = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "gate mygate(a,b,c) q {\n"
                      "\tu1(-(3*(pi-5))/2*(pi+8/1.5)*(1-1.0)+(2+a*(5/"
                      "2-1.5)*(b^(c^(2-1))))) q;\n"
                      "\tu1(sin((4*(3+(2-(1/c))))/(b*(3+(2-1/"
                      "2)))^((0.5^(-2))+(1+(2+(3+4)))^a))) q;\n"
                      "\tu1(ln(tan((4-pi)+(pi-4))-cos(1+2*(c+0)))) q;\n"
                      "\tu1((1*a)+(c/1)) q;\n"
                      "}\n"
                      "qreg q[2];\n"
                      "u1(exp(ln(4+0.5))) q;\n";

    std::string post =
        "OPENQASM 2.0;\n"
        "include \"qelib1.inc\";\n"
        "\n"
        "gate mygate(a,b,c) q {\n"
        "\tu1(2+(a*(b^c))) q;\n"
        "\tu1(sin((4*(3+(2-(1/c))))/((b*(9/2))^(4+(10^a))))) q;\n"
        "\tu1(ln(-cos(1+(2*c)))) q;\n"
        "\tu1(a+c) q;\n"
        "}\n"
        "qreg q[2];\n"
        "u1(4.5) q;\n";

    auto program = parser::parse_string(pre, "mixed.qasm");
    transformations::expr_simplify(*program);
    std::stringstream ss;
    ss << *program;

    EXPECT_EQ(ss.str(), post);
}
/******************************************************************************/
