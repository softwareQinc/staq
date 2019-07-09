/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "utils/source_manager.hpp"
#include "utils/diagnostic.hpp"

#include "ast/ast.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"
#include "token.hpp"
#include "token_kinds.hpp"

#include <string>
#include <vector>
#include <memory>

namespace synthewareQ::qasm {

inline std::unique_ptr<ast_context> read_from_file(std::string path)
{
	source_manager source_manager;
	error_diagnostic_engine diagnostic;
	preprocessor pp_lexer(source_manager, diagnostic);
	parser parser(pp_lexer, source_manager, diagnostic);

	pp_lexer.add_target_file(path);
	auto success = parser.parse();
	return success;
}

inline std::unique_ptr<ast_context> read_from_buffer(std::string buffer)
{
	source_manager source_manager;
	error_diagnostic_engine diagnostic;
	preprocessor pp_lexer(source_manager, diagnostic);
	parser parser(pp_lexer, source_manager, diagnostic);

	pp_lexer.add_target_buffer(buffer);
	auto success = parser.parse();
	return success;
}

  inline std::unique_ptr<ast_context> read_from_stdin(std::streamsize buffer_sz = 65536)
{
	source_manager source_manager;
	error_diagnostic_engine diagnostic;
	preprocessor pp_lexer(source_manager, diagnostic);
	parser parser(pp_lexer, source_manager, diagnostic);

    // Read into a buffer, credit goes to stack overflow for this solution
    std::vector<char> cin_str;
    std::vector<char> buffer(buffer_sz);
    cin_str.reserve(buffer_sz);

    auto rdbuf = std::cin.rdbuf();
    while (auto cnt_char = rdbuf->sgetn(buffer.data(), buffer_sz)) {
      cin_str.insert(cin_str.end(), buffer.data(), buffer.data() + cnt_char);
    }

	pp_lexer.add_target_buffer(std::string(cin_str.begin(), cin_str.end()));
	auto success = parser.parse();
	return success;
}

} // namespace synthewareQ::qasm
