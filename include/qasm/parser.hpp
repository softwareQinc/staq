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
#include "preprocessor.hpp"
#include "token.hpp"
#include "token_kinds.hpp"

#include <fmt/format.h>

namespace synthewareQ {
namespace qasm {

// This implements a parser for Open QASM 2.0. After parsing units of the
// grammar, productions are invoked to handle whatever has been read.
//
// TODO: error recovery.
class parser {
	preprocessor& pp_lexer_;
	source_manager& source_manager_;
	diagnostic_engine& diagnostic_;
	std::unique_ptr<ast_context> context_;

	bool error_ = false;

	// The current token we are peeking.
	token current_token_;

	// The location of the token we previously consumed. This is used
	// for diagnostics in which we expected to see a token following
	// another token (e.g., the ';' at the end of a statement).
	unsigned prev_token_location_;

private:
	// Consume the current token 'current_token_' and lex the next one.
	// Returns the location of the consumed token.
	uint32_t consume_token()
	{
		prev_token_location_ = current_token_.location;
		current_token_ = pp_lexer_.next_token();
		return prev_token_location_;
	}

	// The parser expects that the current token is of 'expected' kind.
	// If it is not, it emits a diagnostic, puts the parser in a error
	// state and returns the current_token_. Otherwise consumes the token
	// and returns it.
	token expect_and_consume_token(token_kinds expected)
	{
		if (error_) {
			return current_token_;
		}
		if (current_token_.is_not(expected)) {
			diagnostic_.report(diagnostic_levels::error,
			                   source_manager_.location_str(current_token_.location),
			                   fmt::format("expected {} but got {}",
			                               token_name(expected), current_token_.name()));
			error_ = true;
			return current_token_;
		}
		auto return_token = current_token_;
		consume_token();
		return return_token;
	}

	// The parser try to see if the current token is of 'expected' kind.
	// If it is not, returns false. Otherwise consumes the token and
	// returns true.
	bool try_and_consume_token(token_kinds expected)
	{
		if (current_token_.is_not(expected) || error_) {
			return false;
		}
		consume_token();
		return true;
	}

public:
	parser(preprocessor& pp_lexer, source_manager& source_manager, diagnostic_engine& diagnostic)
	    : pp_lexer_(pp_lexer)
	    , source_manager_(source_manager)
	    , diagnostic_(diagnostic)
	    , context_(std::make_unique<ast_context>(source_manager, diagnostic))
	{}

	// <mainprogram> = OPENQASM <real> ; <program>
	// <program>     = <statement> | <program> <statement>
	// <statement>   = <decl>
	//               | <gatedecl> <goplist> }
	//               | <gatedecl> }
    //               | <opaquedecl> ;
	//               | <qop>
	//               | if ( <id> == <nninteger> ) <qop>
	//               | barrier <anylist> ;
	std::unique_ptr<ast_context> parse()
	{
		// The first (non-comment) line of an Open QASM program must be
		// OPENQASM M.m; indicating a major version M and minor version m.
		parse_header();
		while (!error_) {
			if (current_token_.is(token_kinds::eof)) {
				break;
			}
			switch (current_token_.kind) {
			// Parse declarations (<decl>)
			case token_kinds::kw_creg: {
				auto node = parse_decl(register_type::classical);
				context_->add_decl_register(node->identifier(), node);
			} break;

			case token_kinds::kw_qreg: {
				auto node = parse_decl(register_type::quantum);
				context_->add_decl_register(node->identifier(), node);
			} break;

			// Parse gate declaration (<gatedecl>)
			case token_kinds::kw_gate: {
				auto node = parse_gatedecl();
				context_->add_decl_gate(node->identifier(), node);
			} break;


            // Parse opaque gate declaration
            case token_kinds::kw_opaque: {
                auto node = parse_opaquedecl();
                context_->add_decl_gate(node->identifier(), node);
            } break;

            // Parse classical gate declaration
            case token_kinds::kw_oracle: {
                auto node = parse_oracledecl();
                context_->add_decl_gate(node->identifier(), node);
            } break;

			// Parse quantum operations (<qop>)
			case token_kinds::identifier:
			case token_kinds::kw_cx:
			case token_kinds::kw_measure:
			case token_kinds::kw_reset:
			case token_kinds::kw_u:
				context_->add_node(parse_qop());
				break;

			case token_kinds::kw_barrier:
				context_->add_node(parse_barrier());
				break;

			case token_kinds::kw_if:
				context_->add_node(parse_if());
				break;

			default:
				error_ = true;
				break;
			}
			if (diagnostic_.num_errors) {
				error_ = true;
			}
		}
		if (error_) {
			return nullptr;
		}
		return std::move(context_);
	}

private:
	/*! \brief Parse file header */
	void parse_header()
	{
		consume_token();
		expect_and_consume_token(token_kinds::kw_openqasm);
		expect_and_consume_token(token_kinds::real);
		expect_and_consume_token(token_kinds::semicolon);
	}

	/*! \brief Parse a register declaration (<decl>) */
	// <decl> = qreg <id> [ <nninteger> ] ;
	//        | creg <id> [ <nninteger> ] ;
	decl_register* parse_decl(register_type type)
	{
		// If we get here, then either 'qreg' or 'creg' was matched
		auto location = current_token_.location;
		consume_token();
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		expect_and_consume_token(token_kinds::l_square);
		auto size = expect_and_consume_token(token_kinds::nninteger);
		expect_and_consume_token(token_kinds::r_square);
		expect_and_consume_token(token_kinds::semicolon);

		if (!error_) {
			return decl_register::build(context_.get(), location, type, identifier, size);
		}
		return nullptr;
	}

	/*! \brief Parse an ancilla (<localdecl>) */
	// <localdecl> = ancilla <id> [ <nninteger> ] ;
	//             | dirty ancilla <id> [ <nninteger> ] ;
	decl_ancilla* parse_localdecl()
	{
		auto location = current_token_.location;
        auto dirty = false;
        if (try_and_consume_token(token_kinds::kw_dirty)) {
          dirty = true;
        }

		expect_and_consume_token(token_kinds::kw_ancilla);
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		expect_and_consume_token(token_kinds::l_square);
		auto size = expect_and_consume_token(token_kinds::nninteger);
		expect_and_consume_token(token_kinds::r_square);
		expect_and_consume_token(token_kinds::semicolon);

		if (!error_) {
          return decl_ancilla::build(context_.get(), location, identifier, size, dirty);
		}
		return nullptr;
	}

	/*! \brief Parse a gate declaration (<gatedecl>) */
	// <gatedecl> = gate <id> <idlist> {
	//            | gate <id> ( ) <idlist> {
	//            | gate <id> ( <idlist> ) <idlist> {
	//
	decl_gate* parse_gatedecl()
	{
		// If we get here, then either 'gate'
		consume_token();
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		auto decl = decl_gate::builder(context_.get(), identifier.location, identifier);

		if (try_and_consume_token(token_kinds::l_paren)) {
			if (not try_and_consume_token(token_kinds::r_paren)) {
				decl.add_parameters(parse_idlist());
				expect_and_consume_token(token_kinds::r_paren);
			}
		}
		decl.add_arguments(parse_idlist());
		expect_and_consume_token(token_kinds::l_brace);
		if (not try_and_consume_token(token_kinds::r_brace)) {
			decl.add_body(parse_goplist());
			expect_and_consume_token(token_kinds::r_brace);
		}
		context_->clear_scope();
		if (!error_) {
			return decl.finish();
		}
		return nullptr;
	}

	/*! \brief Parse an opaque declaration */
	// <opaquedecl> = opaque <id> <idlist> ;
	//              | opaque <id> ( ) <idlist> ; 
	//              | opaque <id> ( <idlist> ) <idlist> ; 
	//
	decl_gate* parse_opaquedecl()
    {
        // Mostly follows gat declarations
		consume_token();
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		auto decl = decl_gate::builder(context_.get(), identifier.location, identifier, gate_type::opaque);

		if (try_and_consume_token(token_kinds::l_paren)) {
			if (not try_and_consume_token(token_kinds::r_paren)) {
				decl.add_parameters(parse_idlist());
				expect_and_consume_token(token_kinds::r_paren);
			}
		}
		decl.add_arguments(parse_idlist());
		expect_and_consume_token(token_kinds::semicolon);
		context_->clear_scope();
		if (!error_) {
			return decl.finish();
		}
		return nullptr;
    }

	/*! \brief Parse an oracle declaration */
	// <oracledecl> = oracle <id> <idlist> { <string> };
	//
	decl_gate* parse_oracledecl()
    {
		consume_token();
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		auto decl = decl_gate::builder(context_.get(), identifier.location, identifier, gate_type::oracle);

		decl.add_arguments(parse_idlist());

        expect_and_consume_token(token_kinds::l_brace);
        auto token = expect_and_consume_token(token_kinds::string);
        std::string_view fname = token;
        decl.add_file(logic_file::build(context_.get(), token.location, fname.substr(1, fname.length() - 2)));
		expect_and_consume_token(token_kinds::r_brace);


		context_->clear_scope();
		if (!error_) {
			return decl.finish();
		}
		return nullptr;
    }

	/*! \brief Parse gate operation list, i.e. the gate body (<goplist>) */
	// <goplist> = ancilla <id> [ <integer> ]
    //           | dirty ancilla <id> [ <integer> ]
    //           | <uop>
	//           | barrier <idlist> ;
	//           | <goplist> <uop>
	//           | <goplist> barrier <idlist> ;
	list_gops* parse_goplist()
	{
		auto builder = list_gops::builder(context_.get(), current_token_.location);
		do {
			switch (current_token_.kind) {
            case token_kinds::kw_ancilla:
            case token_kinds::kw_dirty:
                builder.add_child(parse_localdecl());
                break;

			case token_kinds::kw_cx:
				builder.add_child(parse_cnot());
				break;

			case token_kinds::kw_u:
				builder.add_child(parse_unitary());
				break;

			case token_kinds::identifier:
				builder.add_child(parse_gate_statement());
				break;

			default:
				goto end;
			}
		} while (1);
	end:
		return builder.finish();
	}

	/*! \brief Parse a quantum operation (qop) */
	// <qop> = <uop> 
	//       | measure <argument> -> <argument> ; 
	//       | reset <argument> ;
	ast_node* parse_qop()
	{
		switch (current_token_.kind) {
		case token_kinds::kw_measure:
			return parse_measure();

		case token_kinds::kw_reset:
			return parse_reset();

		case token_kinds::identifier:
		case token_kinds::kw_cx:
		case token_kinds::kw_u:
			return parse_uop();

		default:
			break;
		}
		return nullptr;
	}

	/*! \brief Parse unitary operation (<uop>) */
	// <uop> = U ( <explist> ) <argument> ;
	//       | CX <argument> , <argument> ;
	//       | <id> <anylist> ; 
	//       | <id> ( ) <anylist> ;
	//       | <id> ( <explist> ) <anylist> ;
	ast_node* parse_uop()
	{
		switch (current_token_.kind) {
		case token_kinds::identifier:
			return parse_gate_statement();

		case token_kinds::kw_cx:
			return parse_cnot();

		case token_kinds::kw_u:
			return parse_unitary();

		default:
			break;
		}
		return nullptr;
	}

	/*! \brief Parse a list of arguments (<anylist>) */
	// <anylist> = <idlist> | <mixedlist>
	// <mixedlist> = <id> [ <nninteger> ] | <mixedlist> , <id>
	//             | <mixedlist> , <id> [ <nninteger> ]
	//             | <idlist> , <id>[ <nninteger> ]
	bool parse_anylist(stmt_gate::builder& builder)
	{
		do {
			auto arg = parse_argument();
			if (arg == nullptr) {
				return false;
			}
			builder.add_child(arg);
			if (!try_and_consume_token(token_kinds::comma)) {
			break;
			}
		} while (1);
		return true;
	}

	/*! \brief Parse a identifier (<idlist>) */
	// <idlist> = <id>
	//          | <idlist> , <id>
	list_ids* parse_idlist()
	{
		auto builder = list_ids::builder(context_.get(), current_token_.location);
		do {
			auto identifier = expect_and_consume_token(token_kinds::identifier);
			auto param = decl_param::build(context_.get(), identifier.location, identifier);
			context_->add_decl_parameter(identifier, param);
			builder.add_child(param);
			if (not try_and_consume_token(token_kinds::comma)) {
				break;
			}
		} while (1);
		return builder.finish();
	}

	/*! \brief Parse an argument (<argument>) */
	// <argument> = <id>
	//            | <id> [ <nninteger> ]
	ast_node* parse_argument()
	{
		auto location = current_token_.location;
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		auto declaration_reference = create_decl_reference(location, identifier);
		if (declaration_reference == nullptr) {
			return nullptr;
		}
		if (not try_and_consume_token(token_kinds::l_square)) {
			return declaration_reference;
		}

		auto indexed_reference = expr_reg_idx_ref::builder(context_.get(), location);
		auto idx = expect_and_consume_token(token_kinds::nninteger);
		auto index = expr_integer::create(context_.get(), idx.location, idx);
		expect_and_consume_token(token_kinds::r_square);
		if (!error_) {
			indexed_reference.add_child(declaration_reference);
			indexed_reference.add_child(index);
			return indexed_reference.finish();
		}
		return nullptr;
	}	

	/*! \brief Parse expression list (<explist>) */
	// <explist> = <exp> 
	//           | <explist> , <exp>
	void parse_explist(stmt_gate::builder& builder)
	{
		do {
			auto expr = parse_exp();
			builder.add_child(expr);
			if (not try_and_consume_token(token_kinds::comma)) {
			break;
		}
		} while (1);
		return;
	}

	/*! \brief Parse an expression (<exp>) */
	// <exp> = <real> | <nninteger> | pi | <id>
	//       | <exp> + <exp> | <exp> - <exp> 
	//       | <exp> * <exp> | <exp> / <exp> 
	//       | - <exp> 
	//       | <exp> ^ <exp>
	//       | ( <exp> ) 
	//       | <unaryop> ( <exp> )
	ast_node* parse_exp(unsigned min_precedence = 1)
	{
		ast_node* atom_lhs = parse_atom();
		while (1) {
			auto next_min_precedence = min_precedence;
			binary_ops op = binary_ops::unknown;

			switch (current_token_.kind) {
			case token_kinds::plus:
				if (min_precedence > 1) {
					goto end;
				}
				op = binary_ops::addition;
				next_min_precedence = 2;
				break;

			case token_kinds::minus:
				if (min_precedence > 1) {
					goto end;
				}
				op = binary_ops::subtraction;
				next_min_precedence = 2;
				break;

			case token_kinds::star:
				if (min_precedence > 2) {
					goto end;
				}
				op = binary_ops::multiplication;
				next_min_precedence = 3;
				break;

			case token_kinds::slash:
				if (min_precedence > 2) {
					goto end;
				}
				op = binary_ops::division;
				next_min_precedence = 3;
				break;

			case token_kinds::caret:
				if (min_precedence > 3) {
					goto end;
				}
				op = binary_ops::exponentiation;
				next_min_precedence = 4;
				break;

			default:
				goto end;
			}

			consume_token();
			ast_node* atom_rhs = parse_exp(next_min_precedence);
			auto binary_op = expr_binary_op::builder(context_.get(), current_token_.location, op);
			binary_op.add_child(atom_lhs);
			binary_op.add_child(atom_rhs);
			atom_lhs = binary_op.finish();
		}
	end:
		return atom_lhs;
	}

#pragma region Helper functions
	stmt_cnot* parse_cnot()
	{
		// If we get here 'CX' was matched
		auto location = current_token_.location;
		consume_token();
		auto arg1 = parse_argument();
		expect_and_consume_token(token_kinds::comma);
		auto arg2 = parse_argument();
		expect_and_consume_token(token_kinds::semicolon);
		if (not error_) {
			auto cnot_builder = stmt_cnot::builder(context_.get(), location);
			cnot_builder.add_child(arg1);
			cnot_builder.add_child(arg2);
			return cnot_builder.finish();
		}
		return nullptr;
	}

	bool parse_anylist(stmt_barrier::builder& builder)
	{
		do {
			auto arg = parse_argument();
			if (arg == nullptr) {
				return false;
			}
			builder.add_child(arg);
			if (!try_and_consume_token(token_kinds::comma)) {
				break;
			}
		} while (1);
		return true;
	}

	ast_node* parse_atom()
	{
		if (try_and_consume_token(token_kinds::l_paren)) {
			auto atom = parse_exp();
			expect_and_consume_token(token_kinds::r_paren);
			return atom;
		}
		if (try_and_consume_token(token_kinds::minus)) {
			auto sign = expr_unary_op::builder(context_.get(), current_token_.location, unary_ops::minus);
			auto atom = parse_exp();
			sign.add_child(atom);
			return sign.finish();
		}

		ast_node* atom = nullptr;
		switch (current_token_.kind) {
		case token_kinds::identifier:
			atom = create_decl_reference(current_token_.location, current_token_);
			break;

		case token_kinds::nninteger:
			atom = expr_integer::create(context_.get(), current_token_.location, current_token_);
			break;

		case token_kinds::kw_pi:
			atom = expr_pi::create(context_.get(), current_token_.location);
			break;

		case token_kinds::real:
			atom = expr_real::create(context_.get(), current_token_.location, current_token_);
			break;
		default:
			break;
		}
		if (atom != nullptr) {
			consume_token();
			return atom;
		}

		auto op = unary_ops::unknown;
		switch (current_token_.kind) {
		case token_kinds::kw_uop_sin:
			op = unary_ops::sin;
			break;

		case token_kinds::kw_uop_cos:
			op = unary_ops::cos;
			break;

		case token_kinds::kw_uop_tan:
			op = unary_ops::tan;
			break;

		case token_kinds::kw_uop_exp:
			op = unary_ops::exp;
			break;

		case token_kinds::kw_uop_ln:
			op = unary_ops::ln;
			break;

		case token_kinds::kw_uop_sqrt:
			op = unary_ops::sqrt;
			break;

		default:
			std::cerr << "Error\n";
			return nullptr;
		}

		consume_token();
		auto unary_op = expr_unary_op::builder(context_.get(), current_token_.location, op);
		expect_and_consume_token(token_kinds::l_paren);
		atom = parse_exp();
		expect_and_consume_token(token_kinds::r_paren);
		unary_op.add_child(atom);
		return unary_op.finish();
	}

	stmt_gate* parse_gate_statement()
	{
		// If we get here, then an identifier was matched
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		auto stmt_builder = stmt_gate::builder(context_.get(), identifier.location);
		auto gate_reference = create_decl_reference(identifier.location, identifier);

		stmt_builder.add_child(gate_reference);
		if (try_and_consume_token(token_kinds::l_paren)) {
			if (not try_and_consume_token(token_kinds::r_paren)) {
				parse_explist(stmt_builder);
				expect_and_consume_token(token_kinds::r_paren);
			}
		}
		if (!parse_anylist(stmt_builder)) {
			return nullptr;
		}
		expect_and_consume_token(token_kinds::semicolon);
		if (not error_) {
			return stmt_builder.finish();
		}
		return nullptr;
	}

	stmt_unitary* parse_unitary()
	{
		// If we get here 'U' was matched
		consume_token();
		auto location = current_token_.location;
		expect_and_consume_token(token_kinds::l_paren);
		auto param1 = parse_exp();
		expect_and_consume_token(token_kinds::comma);
		auto param2 = parse_exp();
		expect_and_consume_token(token_kinds::comma);
		auto param3 = parse_exp();
		expect_and_consume_token(token_kinds::r_paren);
		auto target = parse_argument();
		expect_and_consume_token(token_kinds::semicolon);

		if (not error_) {
			auto unitary_builder = stmt_unitary::builder(context_.get(), location);
			unitary_builder.add_child(param1);
			unitary_builder.add_child(param2);
			unitary_builder.add_child(param3);
			unitary_builder.add_child(target);
			return unitary_builder.finish();
		}
		return nullptr;
	}

	stmt_measure* parse_measure()
	{
		// If we get here 'measure' was matched
		auto stmt_builder = stmt_measure::builder(context_.get(), current_token_.location);
		consume_token();
		auto arg0 = parse_argument();
		expect_and_consume_token(token_kinds::arrow);
		auto arg1 = parse_argument();
		expect_and_consume_token(token_kinds::semicolon);
		stmt_builder.add_child(arg0);
		stmt_builder.add_child(arg1);
		return stmt_builder.finish();
	}

	stmt_reset* parse_reset()
	{
		// If we get here 'reset' was matched
		auto stmt_builder = stmt_reset::builder(context_.get(), current_token_.location);
		consume_token();
		auto arg = parse_argument();
		expect_and_consume_token(token_kinds::semicolon);
		stmt_builder.add_child(arg);
		return stmt_builder.finish();
	}

	stmt_barrier* parse_barrier()
	{
		// If we get here 'barrier' was matched
		auto stmt_builder = stmt_barrier::builder(context_.get(), current_token_.location);
		consume_token();
		parse_anylist(stmt_builder);
		expect_and_consume_token(token_kinds::semicolon);
		return stmt_builder.finish();
	}

	stmt_if* parse_if()
	{
		// If we get here 'if' was matched
		auto stmt_builder = stmt_if::builder(context_.get(), current_token_.location);
		consume_token();
		expect_and_consume_token(token_kinds::l_paren);
		auto identifier = expect_and_consume_token(token_kinds::identifier);
		auto declaration_reference = create_decl_reference(identifier.location, identifier);
		expect_and_consume_token(token_kinds::equalequal);
		auto integer = expect_and_consume_token(token_kinds::nninteger);
		auto atom = expr_integer::create(context_.get(), integer.location, integer);
		expect_and_consume_token(token_kinds::r_paren);
		stmt_builder.add_child(declaration_reference);
		stmt_builder.add_child(atom);
		stmt_builder.add_child(parse_qop());
		return stmt_builder.finish();
	}

	expr_decl_ref* create_decl_reference(uint32_t location, std::string_view identifier)
	{
		auto declaration = context_->find_declaration(identifier);
		if (declaration) {
			return expr_decl_ref::build(context_.get(), location, declaration);
		}
		diagnostic_.report(diagnostic_levels::error, source_manager_.location_str(location),
		                   fmt::format("undefined reference to {}", identifier));
		return nullptr;
	}

#pragma endregion
};

} // namespace qasm
} // namespace synthewareQ
