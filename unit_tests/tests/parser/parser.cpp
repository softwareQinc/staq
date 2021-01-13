#ifndef PATH
#define PATH ""
#endif

#include "gtest/gtest.h"
#include "parser/parser.hpp"
#include "ast/semantic.hpp"

#include <sstream>

using namespace staq;

// Parsing & semantic analysis unit tests
/******************************************************************************/
TEST(Parsing, Syntax_All) {
    std::string src = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "qreg q[2];\n"
                      "creg c[2];\n"
                      "opaque a q;\n"
                      "gate b q {\n"
                      "  ancilla a[1];\n"
                      "  dirty ancilla b[1];\n"
                      "}\n"
                      "oracle d q { \"dummy.v\" }\n"
                      "U(0,0,0) q[0];\n"
                      "CX q[0],q[1];\n"
                      "b q[0];\n"
                      "barrier q;\n"
                      "reset q;\n"
                      "measure q -> c;\n"
                      "if(c==1) a q[0];\n";
    EXPECT_NO_THROW(parser::parse_string(src, "syntax_all.qasm"));
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Standard_Gates) {
    std::string src = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "qreg q[3];\n"
                      "u3(0,0,0) q[0];\n"
                      "u2(0,0) q[0];\n"
                      "u1(0) q[0];\n"
                      "cx q[0],q[1];\n"
                      "id q[0];\n"
                      "x q[0];\n"
                      "y q[0];\n"
                      "z q[0];\n"
                      "h q[0];\n"
                      "s q[0];\n"
                      "sdg q[0];\n"
                      "t q[0];\n"
                      "tdg q[0];\n"
                      "rx(0) q[0];\n"
                      "ry(0) q[0];\n"
                      "rz(0) q[0];\n"
                      "cz q[0],q[1];\n"
                      "cy q[0],q[1];\n"
                      "ch q[0],q[1];\n"
                      "ccx q[0],q[1],q[2];\n"
                      "crz(0) q[0],q[1];\n"
                      "cu1(0) q[0],q[1];\n"
                      "cu3(0,0,0) q[0],q[1];\n";
    EXPECT_NO_THROW(parser::parse_string(src, "standard_gates.qasm"));
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Namespaces) {
    std::string src = "OPENQASM 2.0;\n"
                      "opaque x y;\n"
                      "qreg x[1];\n"
                      "x x;\n";
    EXPECT_NO_THROW(parser::parse_string(src, "namespaces.qasm"));
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Type_Error_Cbit) {
    std::string src = "OPENQASM 2.0;\n"
                      "creg x[1];\n"
                      "U(0,0,0) x[0];\n";
    EXPECT_THROW(parser::parse_string(src, "type_error_cbit.qasm"),
                 ast::SemanticError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Type_Error_Real) {
    std::string src = "OPENQASM 2.0;\n"
                      "gate bad(x) y {\n"
                      "  U(0,0,0) x;\n"
                      "}\n";
    EXPECT_THROW(parser::parse_string(src, "type_error_real.qasm"),
                 ast::SemanticError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Mapping_Pass) {
    std::string src = "OPENQASM 2.0;\n"
                      "qreg x[2];\n"
                      "qreg y[2];\n"
                      "CX x, y;\n";
    EXPECT_NO_THROW(parser::parse_string(src, "mapping_pass.qasm"));
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Mapping_Fail) {
    std::string src = "OPENQASM 2.0;\n"
                      "qreg x[1];\n"
                      "qreg y[2];\n"
                      "CX x, y;\n";
    EXPECT_THROW(parser::parse_string(src, "mapping_fail.qasm"),
                 ast::SemanticError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Repeated_Arguments1) {
    std::string src = "OPENQASM 2.0;\n"
                      "qreg x[1];\n"
                      "CX x[0], x[0];\n";
    EXPECT_THROW(parser::parse_string(src, "repeated_arguments1.qasm"),
                 ast::SemanticError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Repeated_Arguments2) {
    std::string src = "OPENQASM 2.0;\n"
                      "qreg x[1];\n"
                      "CX x, x[0];\n";
    EXPECT_THROW(parser::parse_string(src, "repeated_arguments2.qasm"),
                 ast::SemanticError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Repeated_Arguments3) {
    std::string src = "OPENQASM 2.0;\n"
                      "qreg x[1];\n"
                      "CX x[0], x;\n";
    EXPECT_THROW(parser::parse_string(src, "repeated_arguments3.qasm"),
                 ast::SemanticError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Standard_Compliance) {
    // generic circuits
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/adder.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/bigadder.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/inverseqft1.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/inverseqft2.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/ipea_3_pi_8.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/pea_3_pi_8.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/qec.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/qft.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/qpt.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/rb.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/teleport.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/teleportv2.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/generic/W-state.qasm"));

    // ibmqx2 circuits
    EXPECT_NO_THROW(
        parser::parse_file(PATH "/qasm/ibmqx2/011_3_qubit_grover_50_.qasm"));
    EXPECT_NO_THROW(
        parser::parse_file(PATH "/qasm/ibmqx2/Deutsch_Algorithm.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/iswap.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/qe_qft_3.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/qe_qft_4.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/qe_qft_5.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/W3test.qasm"));

    // invalid circuits
    EXPECT_THROW(parser::parse_file(PATH "/qasm/invalid/gate_no_found.qasm"),
                 ast::SemanticError);
    EXPECT_THROW(
        parser::parse_file(PATH "/qasm/invalid/missing_semicolon.qasm"),
        parser::ParseError);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Idempotence) {
    std::string src = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[2];\n"
                      "creg c[2];\n"
                      "opaque a q;\n"
                      "gate b q {\n"
                      "\tancilla a[1];\n"
                      "\tdirty ancilla b[1];\n"
                      "}\n"
                      "oracle d q { \"dummy.v\" }\n"
                      "U(0,0,0) q[0];\n"
                      "CX q[0],q[1];\n"
                      "b q[0];\n"
                      "barrier q;\n"
                      "reset q;\n"
                      "measure q -> c;\n"
                      "if (c==1) a q[0];\n";

    auto prog = parser::parse_string(src, "idempotence_test.qasm");
    std::stringstream ss;
    ss << *prog;

    EXPECT_EQ(ss.str(), src);
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Trailing_Comment) {
    std::string src = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "//";

    EXPECT_NO_THROW(parser::parse_string(src, "trailing_comment.qasm"));
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Scientific_Notation) {
    std::string src = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "U(0.01e3,.02E+4,1.54E-10) q[0];\n"
                      "U(1E3, 1e-3, 0.E1) q[0];\n";

    EXPECT_NO_THROW(parser::parse_string(src, "scientific_notation.qasm"));
}
/******************************************************************************/

/******************************************************************************/
TEST(Parsing, Unary_Plus) {
    std::string src = "OPENQASM 2.0;\n"
                      "include \"qelib1.inc\";\n"
                      "\n"
                      "qreg q[1];\n"
                      "U(+1,+(1+1),+1+(+1)) q[0];\n";

    EXPECT_NO_THROW(parser::parse_string(src, "unary_plus.qasm"));
}
/******************************************************************************/
