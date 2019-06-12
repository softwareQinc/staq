/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <unordered_map>
#include <string>

namespace synthewareQ {
namespace qasm {

// ast node kinds.
enum class ast_node_kinds : unsigned short {
	#define AST_NODE(X) X,
	#include "ast_node_kinds.def"
};

static const char * const ast_node_names[] = {
	#define AST_NODE(X) #X,
	#include "ast_node_kinds.def"
	nullptr
};

// Determines the name of a ast node as used within the front end.
// The name of a ast node will be an internal name.
const char *ast_node_name(ast_node_kinds k)
{
	auto k_idx = static_cast<int>(k);
	return ast_node_names[k_idx];
}

} // namespace qams
} // namespace synthewareQ
