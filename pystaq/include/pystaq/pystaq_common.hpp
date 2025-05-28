/*
 * This file is part of pystaq.
 *
 * Copyright (c) 2019 - 2025 softwareQ Inc. All rights reserved.
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

#ifndef PYSTAQ_COMMON_H_
#define PYSTAQ_COMMON_H_

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "qasmtools/parser/parser.hpp"

#include "staq/transformations/barrier_merge.hpp"
#include "staq/transformations/desugar.hpp"
#include "staq/transformations/expression_simplifier.hpp"
#include "staq/transformations/inline.hpp"
#include "staq/transformations/oracle_synthesizer.hpp"

#ifdef GRID_SYNTH
#include "staq/grid_synth/grid_synth.hpp"
#include "staq/grid_synth/types.hpp"
#include "staq/transformations/qasm_synth.hpp"
#endif

#include "staq/optimization/cnot_resynthesis.hpp"
#include "staq/optimization/rotation_folding.hpp"
#include "staq/optimization/simplify.hpp"

#include "staq/mapping/device.hpp"
#include "staq/mapping/layout/basic.hpp"
#include "staq/mapping/layout/bestfit.hpp"
#include "staq/mapping/layout/eager.hpp"
#include "staq/mapping/mapping/steiner.hpp"
#include "staq/mapping/mapping/swap.hpp"

#include "staq/tools/qubit_estimator.hpp"
#include "staq/tools/resource_estimator.hpp"

#include "staq/output/cirq.hpp"
#include "staq/output/ionq.hpp"
#include "staq/output/json.hpp"
#include "staq/output/lattice_surgery.hpp"
#include "staq/output/projectq.hpp"
#include "staq/output/qsharp.hpp"
#include "staq/output/quil.hpp"

namespace py = pybind11;

#endif /* PYSTAQ_COMMON_H_ */
