/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "ast_context.hpp"
#include "ast_node.hpp"
#include "ast_node_kinds.hpp"

// Nodes
#include "nodes/expr_decl_ref.hpp"
#include "nodes/expr_reg_idx_ref.hpp"
#include "nodes/decl_gate.hpp"
#include "nodes/decl_param.hpp"
#include "nodes/decl_register.hpp"
#include "nodes/decl_program.hpp"
#include "nodes/list_gops.hpp"
#include "nodes/list_ids.hpp"
#include "nodes/stmt_barrier.hpp"
#include "nodes/stmt_cnot.hpp"
#include "nodes/stmt_gate.hpp"
#include "nodes/stmt_if.hpp"
#include "nodes/stmt_measure.hpp"
#include "nodes/stmt_unitary.hpp"
#include "nodes/stmt_reset.hpp"

// Expression
#include "nodes/expr_binary_op.hpp"
#include "nodes/expr_integer.hpp"
#include "nodes/expr_pi.hpp"
#include "nodes/expr_real.hpp"
#include "nodes/expr_unary_op.hpp"

// Extensions
#include "nodes/logic_file.hpp"
#include "nodes/decl_ancilla.hpp"
