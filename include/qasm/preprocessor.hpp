/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "utils/source.hpp"
#include "utils/source_manager.hpp"

#include "lexer.hpp"
#include "token.hpp"
#include "token_kinds.hpp"

#include <fmt/format.h>
#include <memory>
#include <vector>

namespace synthewareQ {
namespace qasm {

static const std::string std_include
    = "gate u3(theta,phi,lambda) q { U(theta,phi,lambda) q; }gate u2(phi,lambda) q { "
      "U(pi/2,phi,lambda) q; }gate u1(lambda) q { U(0,0,lambda) q; }gate cx c,t { CX c,t; }gate id "
      "a { U(0,0,0) a; }gate u0(gamma) q { U(0,0,0) q; }gate x a { u3(pi,0,pi) a; }gate y a { "
      "u3(pi,pi/2,pi/2) a; }gate z a { u1(pi) a; }gate h a { u2(0,pi) a; }gate s a { u1(pi/2) a; "
      "}gate sdg a { u1(-pi/2) a; }gate t a { u1(pi/4) a; }gate tdg a { u1(-pi/4) a; }gate "
      "rx(theta) a { u3(theta, -pi/2,pi/2) a; }gate ry(theta) a { u3(theta,0,0) a; }gate rz(phi) a "
      "{ u1(phi) a; }gate cz a,b { h b; cx a,b; h b; }gate cy a,b { sdg b; cx a,b; s b; }gate swap "
      "a,b { cx a,b; cx b,a; cx a,b; }gate ch a,b {h b; sdg b;cx a,b;h b; t b;cx a,b;t b; h b; s "
      "b; x b; s a;}gate ccx a,b,c{  h c;  cx b,c; tdg c;  cx a,c; t c;  cx b,c; tdg c;  cx a,c; t "
      "b; t c; h c;  cx a,b; t a; tdg b;  cx a,b;}gate crz(lambda) a,b{  u1(lambda/2) b;  cx a,b;  "
      "u1(-lambda/2) b;  cx a,b;}gate cu1(lambda) a,b{  u1(lambda/2) a;  cx a,b;  u1(-lambda/2) b; "
      " cx a,b;  u1(lambda/2) b;}gate cu3(theta,phi,lambda) c, t{  u1((lambda-phi)/2) t;  cx c,t;  "
      "u3(-theta/2,0,-(phi+lambda)/2) t;  cx c,t;  u3(theta/2,phi,0) t;}";

// This is the class able to handle include's. You see, lexers know only about
// tokens within a single source file.
class preprocessor {
	using LexerPtr = std::unique_ptr<lexer>;

	source_manager& source_manager_;
	diagnostic_engine& diagnostic_;

	std::vector<LexerPtr> lexer_stack_;
	// The current top of the stack that we're lexing from.
	LexerPtr current_lexer_ = nullptr;

public:
	preprocessor(source_manager& source_manager, diagnostic_engine& diagnostic)
	    : source_manager_(source_manager)
	    , diagnostic_(diagnostic)
	{}

	// FIXME: For now this is like this because the function to
	// open files does not accept 'string_view' as parameter
	bool add_target_file(const std::string& file_path)
	{
		auto source = source_manager_.add_target_file(file_path);
		if (source == nullptr) {
			return false;
		}
		if (current_lexer_ != nullptr) {
			lexer_stack_.push_back(std::move(current_lexer_));
		}
		current_lexer_ = std::make_unique<lexer>(source->offset(), source->content());
		return true;
	}

	void add_target_buffer(const std::string_view buffer)
	{
		auto source = source_manager_.add_target_buffer(buffer);
		if (current_lexer_ != nullptr) {
			lexer_stack_.push_back(std::move(current_lexer_));
		}
		current_lexer_ = std::make_unique<lexer>(source->offset(), source->content());
	}

	token next_token()
	{
		if (current_lexer_ == nullptr) {
			std::cerr << "No target to lex.\n";
			return {};
		}
		auto token = current_lexer_->next_token();
		if (token.kind == token_kinds::pp_include) {
			handle_include();
			token = current_lexer_->next_token();
		} else if (token.kind == token_kinds::eof) {
			if (not lexer_stack_.empty()) {
				current_lexer_ = std::move(lexer_stack_.back());
				lexer_stack_.pop_back();
				token = current_lexer_->next_token();
			} else {
				current_lexer_ = nullptr;
			}
		}
		return token;
	}

private:
	// The "include" tokens have just been read, read the file to be included
	// from the lexer, then include it!
	void handle_include()
	{
		auto token = current_lexer_->next_token();
		if (token.kind != token_kinds::string) {
			std::cerr << "Include must be followed by a file name\n";
		}
		std::string_view target = token;
		token = current_lexer_->next_token();
		if (token.kind != token_kinds::semicolon) {
			std::cerr << "Missing a ';'\n";
		}
		std::string filename(target.substr(1, target.length() - 2));
		if (add_target_file(filename)) {
			return;
		}
		if (filename == "qelib1.inc") {
			add_target_buffer(std_include);
			diagnostic_.emit(diagnostic_levels::note, "Using internal 'qelib1.inc'");
			return;
		}
		diagnostic_.report(diagnostic_levels::error,
		                   source_manager_.location_str(token.location),
		                   fmt::format("Couldn't find file: {}", filename));
	}
};

} // namespace qasm
} // namespace synthewareQ
