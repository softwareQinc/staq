/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <string>
#include <unordered_map>

namespace synthewareQ {
namespace qasm {

// Tokens from QASM 2.0 language.
enum class token_kinds : unsigned short {
	#define TOKEN(X) X,
	#include "tokens.def"
};

static const std::string token_names[] = {
	#define TOKEN(X) #X,
	#include "tokens.def"
};

static const std::unordered_map<std::string, token_kinds> pp_tokens = {
	#define TOKEN(X)
	#define PPKEYWORD(X) {#X, token_kinds::pp_ ## X},
	#include "tokens.def"
};

static const std::unordered_map<std::string, token_kinds> kw_tokens = {
	#define TOKEN(X)
	#define KEYWORD(X,Y) {Y, token_kinds::kw_ ## X},
	#define UOPERATOR(X,Y) {Y, token_kinds::kw_uop_ ## X},
	#include "tokens.def"
};

// Determines the name of a token as used within the front end.
// The name of a token will be an internal name.
static std::string_view token_name(token_kinds k)
{
	auto k_idx = static_cast<unsigned short>(k);
	return token_names[k_idx];
}

} // namespace qasm
} // namespace synthewareQ
