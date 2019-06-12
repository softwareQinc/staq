/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "token.hpp"
#include "token_kinds.hpp"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace synthewareQ {
namespace qasm {

// lexer class provides a simple interface that turns a text buffer into a
// stream of tokens. This provides no support for file reading or buffering,
// or buffering/seeking of tokens, only forward lexing is supported.
//
// The lexer does't return tokens for every character in the file, it skips
// whitespace and comments.
class lexer {
	std::uint32_t start_location_;
	std::string_view buffer_;
	// Current pointer into the buffer. (Next character to be lexed)
	const char* buffer_ptr_;

public:
	lexer(const lexer&) = delete;
	lexer& operator=(const lexer&) = delete;

	// Create a new lexer object for the specified buffer. This lexer
	// assumes that the associated file buffer will outlive it, so it
	// doesn't take ownership of it-hence 'string_view'.
	lexer(std::uint32_t start_location, std::string_view content)
	    : start_location_(start_location)
	    , buffer_(content)
	    , buffer_ptr_(buffer_.begin())
	{}

	// Lex a token and consume it.
	token next_token()
	{
		return lex();
	}

private:
	// Return current token location.
	std::uint32_t current_location() const
	{
		return start_location_ + (buffer_ptr_ - buffer_.begin());
	}

	// Skip over a series of whitespace characters. Update buffer_ptr_ to
	// point to the next non-whitespace character and return.
	bool skip_whitespace(const char* cur_ptr)
	{
		if ((*cur_ptr == ' ') || (*cur_ptr == '\t')) {
			++cur_ptr;
			while ((*cur_ptr == ' ') || (*cur_ptr == '\t')) {
				++cur_ptr;
			}
			buffer_ptr_ = cur_ptr;
			return true;
		}
		return false;
	}

	// We have just read the // characters from input. Skip until we find
	// the newline or EOF character thats terminate the comment. Then update
	// buffer_ptr_ and return.
	bool skip_line_comment(const char* cur_ptr)
	{
		while (*cur_ptr != 0 && *cur_ptr != '\n' && *cur_ptr != '\r') {
			++cur_ptr;
		}
		buffer_ptr_ = ++cur_ptr;
		return true;
	}

	// When we lex a identifier or a numeric constant token, the token is formed
	// by a span of chars starting at buffer_prt and going till token_end. This
	// method takes that range and assigns it to the token as its location and
	// size. It also update buffer_ptr_.
	token create_token(const char* token_end, token_kinds kind)
	{
		std::uint32_t token_len = token_end - buffer_ptr_;
		buffer_ptr_ = token_end;
		return token(kind, current_location() - token_len, token_len,
		             buffer_ptr_ - token_len);
	}

	// Match [_A-Za-z0-9]*, we have already matched [0-9$]
	token lex_numeric_constant(const char* cur_ptr)
	{
		while (std::isdigit(*cur_ptr)) {
			++cur_ptr;
		}
		if (*cur_ptr != '.') {
			return create_token(cur_ptr, token_kinds::nninteger);
		}
		++cur_ptr;
		while (std::isdigit(*cur_ptr)) {
			++cur_ptr;
		}
		return create_token(cur_ptr, token_kinds::real);
	}

	// Match [_A-Za-z0-9]*, we have already matched [a-z$]
	token lex_identifier(const char* cur_ptr)
	{
		while (std::isalpha(*cur_ptr) || std::isdigit(*cur_ptr) || *cur_ptr == '_') {
			++cur_ptr;
		}
		// Check if the identifier is a known keyword
		auto keyword = kw_tokens.find(std::string(buffer_ptr_, cur_ptr));
		if (keyword != kw_tokens.end()) {
			return create_token(cur_ptr, keyword->second);
		}
		// Check if the identifier is a known preprocessor keyword
		keyword = pp_tokens.find(std::string(buffer_ptr_, cur_ptr));
		if (keyword != pp_tokens.end()) {
			return create_token(cur_ptr, keyword->second);
		}
		return create_token(cur_ptr, token_kinds::identifier);
	}

	// Return the next token in the buffer. If this is the end of buffer, it
	// return the EOF token.
	token lex()
	{
	lex_next_token:
		skip_whitespace(buffer_ptr_);
		auto cur_ptr = buffer_ptr_;
		// Read a character, advancing over it.
		auto tok = token(token_kinds::unknown, current_location(), 1, nullptr);
		auto c = *cur_ptr++;

		switch (c) {
		case 0:
			tok.kind = token_kinds::eof;
			break;

		case '\r':
			if (*cur_ptr == '\n')
				++cur_ptr;
			// FALLTHROUGH
		case '\n':
			buffer_ptr_ = cur_ptr;
			goto lex_next_token;

		case '/':
			if (*cur_ptr == '/') {
				skip_line_comment(cur_ptr);
				goto lex_next_token;
			}
			tok.kind = token_kinds::slash;
			break;

		// clang-format off
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return lex_numeric_constant(cur_ptr);
		// clang-format on

		case 'C':
			if (*cur_ptr == 'X') {
				++cur_ptr;
				tok.kind = token_kinds::kw_cx;
				tok.length = 2;
			}
			break;

		case 'U':
			tok.kind = token_kinds::kw_u;
			break;

		// clang-format off
		case 'O':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
		case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
		case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
		case 'v': case 'w': case 'x': case 'y': case 'z':
			return lex_identifier(cur_ptr);
		// clang-format on

		case '[':
			tok.kind = token_kinds::l_square;
			break;

		case ']':
			tok.kind = token_kinds::r_square;
			break;

		case '(':
			tok.kind = token_kinds::l_paren;
			break;

		case ')':
			tok.kind = token_kinds::r_paren;
			break;

		case '{':
			tok.kind = token_kinds::l_brace;
			break;

		case '}':
			tok.kind = token_kinds::r_brace;
			break;

		case '*':
			tok.kind = token_kinds::star;
			break;

		case '+':
			tok.kind = token_kinds::plus;
			break;

		case '-':
			if (*cur_ptr == '>') {
				++cur_ptr;
				tok.kind = token_kinds::arrow;
				tok.length = 2;
				break;
			}
			tok.kind = token_kinds::minus;
			break;

		case '^':
			tok.kind = token_kinds::caret;
			break;

		case ';':
			tok.kind = token_kinds::semicolon;
			break;

		case '=':
			if (*cur_ptr == '=') {
				++cur_ptr;
				tok.kind = token_kinds::equalequal;
				tok.length = 2;
			}
			break;

		case ',':
			tok.kind = token_kinds::comma;
			break;

		case '"':
			while (*cur_ptr != '"' && *cur_ptr != '\n' && *cur_ptr != '\r') {
				++cur_ptr;
			}
			if (*cur_ptr != '"') {
				std::cerr << "Unmatched \", strings must on the same line\n";
				break;
			}
			++cur_ptr;
			return create_token(cur_ptr, token_kinds::string);

		default:
			break;
		}
		buffer_ptr_ = cur_ptr;
		return tok;
	}
};

} // namespace qasm
} // namespace synthewareQ
