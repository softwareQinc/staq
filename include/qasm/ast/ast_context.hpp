/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "utils/allocators.hpp"
#include "utils/diagnostic.hpp"
#include "utils/source_manager.hpp"

#include "ast_node.hpp"
#include "nodes/decl_program.hpp"

#include "fmt/format.h"

#include <memory>
#include <unordered_map>

namespace synthewareQ {
namespace qasm {

// Forward declaration
class decl_program;

/*! \brief Holds long-lived AST nodes */
class ast_context {
public:
	ast_context(ast_context const&) = delete;
	ast_context& operator=(ast_context const&) = delete;

	ast_context(source_manager& source_manager, diagnostic_engine& diagnostic)
	    : source_manager_(source_manager)
	    , diagnostic_(diagnostic)
	{}

	~ast_context() = default;

	void* allocate(size_t size, uint32_t align = 8) const
	{
		return allocator_.allocate(size, align);
	}

	template<typename T>
	T* allocate(size_t num = 1) const
	{
		return static_cast<T*>(allocate(num * sizeof(T), alignof(T)));
	}

	void deallocate(void*) const
	{}

	void add_node(ast_node* node)
	{
		if (node == nullptr) {
			return;
		}
		program_.add_child(node);
	}

	void add_decl_gate(std::string_view identifier, ast_node* node)
	{
		if (node == nullptr) {
			return;
		}
		identifier_table_.insert({identifier, node});
		program_.add_child(node);
	}

	void add_decl_register(std::string_view identifier, ast_node* node)
	{
		if (node == nullptr) {
			return;
		}
		identifier_table_.insert({identifier, node});
		program_.add_child(node);
	}

	void add_decl_parameter(std::string_view identifier, ast_node* node)
	{
		if (node == nullptr) {
			return;
		}
		bool ok = scope_.insert({identifier, node}).second;
		if (!ok) {
			diagnostic_.report(diagnostic_levels::error,
			                   source_manager_.location_str(node->location()),
			                   fmt::format("redefinition of {}", identifier));
		}
	}

	void add_local(std::string_view identifier, ast_node* node)
	{
		if (node == nullptr) {
			return;
		}
		bool ok = scope_.insert({identifier, node}).second;
		if (!ok) {
			diagnostic_.report(diagnostic_levels::error,
			                   source_manager_.location_str(node->location()),
			                   fmt::format("redefinition of {}", identifier));
		}
	}

	ast_node* find_declaration(std::string_view identifier)
	{
		auto param = scope_.find(identifier);
		if (param != scope_.end()) {
			return param->second;
		}
		auto entry = identifier_table_.find(identifier);
		if (entry != identifier_table_.end()) {
			return entry->second;
		}
		return nullptr;
	}

	void print_allocator_stats() const
	{
		allocator_.print_stats();
	}

	void clear_scope()
	{
		scope_.clear();
	}

	decl_program* root() 
	{
		return &(program_.get());
	}

private:
	/*! \brief The associated source manager. */
	source_manager& source_manager_;

	/*! \brief The associated diagnostic engine. */
	diagnostic_engine& diagnostic_;

	/*! \brief The root node of the AST (a decl_program declaration). */
	decl_program::builder program_;

	/*! \brief The allocator used to create AST objects. */
	// AST objects will be released _only_ when the ast_context itself is destroyed.
	mutable bump_allocator allocator_;

	std::unordered_map<std::string_view, ast_node*> identifier_table_;
	std::unordered_map<std::string_view, ast_node*> scope_;
};

} // namespace qasm
} // namespace synthewareQ

inline void* operator new(size_t bytes, synthewareQ::qasm::ast_context const& ctx,
                          size_t alignment = 8)
{
	return ctx.allocate(bytes, alignment);
}

inline void operator delete(void* ptr, synthewareQ::qasm::ast_context const& ctx, size_t)
{
	ctx.deallocate(ptr);
}

inline void* operator new[](size_t bytes, synthewareQ::qasm::ast_context const& ctx,
                            size_t alignment /* = 8 */)
{
	return ctx.allocate(bytes, alignment);
}

inline void operator delete[](void* ptr, synthewareQ::qasm::ast_context const& ctx, size_t)
{
	ctx.deallocate(ptr);
}
