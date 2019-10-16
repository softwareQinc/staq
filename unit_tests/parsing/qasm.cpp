/*
 * This file is part of synthewareQ.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef PATH 
#define PATH "" 
#endif

#include "gtest/gtest.h"
#include "parser/parser.hpp"
#include "ast/semantic.hpp"

using namespace synthewareQ;

// Parsing & semantic analysis unit tests

/******************************************************************************/
TEST(qasm_parsing, StdCompliance) {
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
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/011_3_qubit_grover_50_.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/Deutsch_Algorithm.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/iswap.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/qe_qft_3.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/qe_qft_4.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/qe_qft_5.qasm"));
    EXPECT_NO_THROW(parser::parse_file(PATH "/qasm/ibmqx2/W3test.qasm"));

    // invalid circuits
    EXPECT_THROW(ast::check_source(*parser::parse_file(PATH "/qasm/invalid/gate_no_found.qasm")),
                 ast::SemanticError);
    EXPECT_THROW(parser::parse_file(PATH "/qasm/invalid/missing_semicolon.qasm"), parser::ParseError);
}
/******************************************************************************/
