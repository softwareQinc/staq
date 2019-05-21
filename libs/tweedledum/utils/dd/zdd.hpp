/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tweedledum {

// TODO: refactor, document
class zdd_base {
private:
	struct node_t {
		node_t(uint32_t var, uint32_t lo, uint32_t hi)
		    : var(var)
		    , ref(0)
		    , dead(0)
		    , lo(lo)
		    , hi(hi)
		{}

		uint64_t var : 12;
		uint64_t ref : 11;
		uint64_t dead : 1;
		uint64_t lo : 20;
		uint64_t hi : 20;
	};

	enum op_t {
		zdd_union,
		zdd_intersection,
		zdd_difference,
		zdd_join,
		zdd_nonsupersets,
		zdd_edivide,
		zdd_sym,
		zdd_choose
	};

public:
	using node = uint32_t; // std::vector<node_t>::size_type;

	explicit zdd_base(uint32_t num_vars, uint32_t log_num_objs = 16)
	    : unique_table(num_vars)
	{
		nodes.reserve(1u << log_num_objs);

		nodes.emplace_back(num_vars, 0, 0);
		nodes.emplace_back(num_vars, 1, 1);

		for (auto v = 0u; v < num_vars; ++v) {
			ref(unique(v, 0, 1));
		}
	}

	node bot() const
	{
		return 0u;
	}

	node top() const
	{
		return 1u;
	}

	node elementary(uint32_t var) const
	{
		return var + 2u;
	}

	/*! \brief Build and store tautology functions
	 *
	 * This function needs to be called before any other node is created,
	 * right after the constructor.
	 */
	void build_tautologies()
	{
		assert(nodes.size() == unique_table.size() + 2u);

		auto last = top();
		for (int v = unique_table.size() - 1; v >= 0; --v) {
			last = unique(v, last, last);
			assert(last == 2 * unique_table.size() + 1u - v);
		}
		ref(last);
	}

	node tautology(uint32_t var = 0) const
	{
		if (var == unique_table.size())
			return 1;
		return 2 * unique_table.size() + 1u - var;
	}

	node union_(node f, node g)
	{
		if (f == 0)
			return g;
		if (g == 0)
			return f;
		if (f == g)
			return f;
		if (f > g)
			std::swap(f, g);

		const auto it = compute_table.find({f, g, zdd_union});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		auto const& F = nodes[f];
		auto const& G = nodes[g];

		node r_lo, r_hi;

		if (F.var < G.var) {
			r_lo = union_(F.lo, g);
			r_hi = F.hi;
		} else if (F.var > G.var) {
			r_lo = union_(f, G.lo);
			r_hi = G.hi;
		} else {
			r_lo = union_(F.lo, G.lo);
			r_hi = union_(F.hi, G.hi);
		}

		const auto var = std::min(F.var, G.var);
		return compute_table[{f, g, zdd_union}] = unique(var, r_lo, r_hi);
	}

	node intersection(node f, node g)
	{
		if (f == 0)
			return 0;
		if (g == 0)
			return 0;
		if (f == g)
			return f;
		if (f > g)
			std::swap(f, g);

		auto const& F = nodes[f];
		auto const& G = nodes[g];

		if (F.var < G.var) {
			return intersection(F.lo, g);
		} else if (F.var > G.var) {
			return intersection(f, G.lo);
		}

		const auto it = compute_table.find({f, g, zdd_intersection});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		const auto r_lo = intersection(F.lo, G.lo);
		const auto r_hi = intersection(F.hi, G.hi);
		return compute_table[{f, g, zdd_intersection}] = unique(F.var, r_lo, r_hi);
	}

	node difference(node f, node g)
	{
		if (f == 0)
			return 0;
		if (f == g)
			return 0;
		if (g == 0)
			return f;

		auto const& F = nodes[f];
		auto const& G = nodes[g];

		if (G.var < F.var)
			return difference(f, G.lo);

		const auto it = compute_table.find({f, g, zdd_difference});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		node r_lo, r_hi;
		if (F.var == G.var) {
			r_lo = difference(F.lo, G.lo);
			r_hi = difference(F.hi, G.hi);
		} else {
			r_lo = difference(F.lo, g);
			r_hi = F.hi;
		}
		return compute_table[{f, g, zdd_difference}] = unique(F.var, r_lo, r_hi);
	}

	node join(node f, node g)
	{
		if (f == 0)
			return 0;
		if (g == 0)
			return 0;
		if (f == 1)
			return g;
		if (g == 1)
			return f;
		if (f > g)
			std::swap(f, g);

		const auto it = compute_table.find({f, g, zdd_join});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		auto const& F = nodes[f];
		auto const& G = nodes[g];

		node r_lo, r_hi;

		if (F.var < G.var) {
			r_lo = join(F.lo, g);
			r_hi = join(F.hi, g);
		} else if (F.var > G.var) {
			r_lo = join(f, G.lo);
			r_hi = join(f, G.hi);
		} else {
			r_lo = join(F.lo, G.lo);
			const auto r_lh = join(F.lo, G.hi);
			const auto r_hl = join(F.hi, G.lo);
			const auto r_hh = join(F.hi, G.hi);
			r_hi = union_(r_lh, union_(r_hl, r_hh));
		}

		const auto var = std::min(F.var, G.var);
		return compute_table[{f, g, zdd_join}] = unique(var, r_lo, r_hi);
	}

	node nonsupersets(node f, node g)
	{
		if (g == 0)
			return f;
		if (f == 0)
			return 0;
		if (g == 1)
			return 0;
		if (f == g)
			return 0;

		auto const& F = nodes[f];
		auto const& G = nodes[g];

		if (F.var > G.var)
			return nonsupersets(f, G.lo);

		const auto it = compute_table.find({f, g, zdd_nonsupersets});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		node r_lo, r_hi;
		if (F.var < G.var) {
			r_lo = nonsupersets(F.lo, g);
			r_hi = nonsupersets(F.hi, g);
		} else {
			r_hi = intersection(nonsupersets(F.hi, G.lo), nonsupersets(F.hi, G.hi));
			r_lo = nonsupersets(F.lo, G.lo);
		}

		return compute_table[{f, g, zdd_nonsupersets}] = unique(F.var, r_lo, r_hi);
	}

	node edivide(node f, node g)
	{
		auto const& F = nodes[f];
		auto const& G = nodes[g];

		if (F.var == G.var) {
			return F.hi;
		}

		if (F.var > G.var) {
			return 0;
		}

		const auto it = compute_table.find({f, g, zdd_edivide});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		node r_lo = edivide(F.lo, g);
		node r_hi = edivide(F.hi, g);
		return compute_table[{f, g, zdd_edivide}] = unique(F.var, r_lo, r_hi);
	}

	node sym(node f, uint32_t v, uint32_t k)
	{
		while (nodes[f].var < v) {
			f = nodes[f].lo;
		}

		if (f <= 1)
			return k > 0 ? 0 : tautology(v);

		const auto it = compute3_table.find({f, v, k, zdd_sym});
		if (it != compute3_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		auto const& F = nodes[f];
		auto r = sym(f, F.var + 1, k);
		if (k > 0) {
			auto q = sym(F.lo, F.var + 1, k - 1);
			r = unique(F.var, r, q);
		}

		auto var = F.var;
		while (var > v) {
			r = unique(--var, r, r);
		}
		return compute3_table[{f, v, k, zdd_sym}] = r;
	}

	node choose(node f, uint32_t k)
	{
		if (k == 1)
			return f;
		if (f <= 1)
			return k > 0 ? 0 : 1;

		const auto it = compute_table.find({f, k, zdd_choose});
		if (it != compute_table.end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		auto const& F = nodes[f];
		auto r = choose(F.lo, k);
		if (k > 0) {
			auto q = choose(F.lo, k - 1);
			r = unique(F.var, r, q);
		}

		return compute_table[{f, k, zdd_choose}] = r;
	}

public:
	uint64_t count_sets(node f) const
	{
		if (f <= 1)
			return f;

		std::unordered_map<node, uint64_t> visited;
		return count_sets_rec(f, visited);
	}

	uint64_t count_nodes(node f) const
	{
		if (f <= 1)
			return 0;

		std::unordered_set<node> visited;
		count_nodes_rec(f, visited);
		return visited.size();
	}

private:
	uint64_t count_sets_rec(node f, std::unordered_map<node, uint64_t>& visited) const
	{
		if (f <= 1)
			return f;
		const auto it = visited.find(f);
		if (it != visited.end()) {
			return it->second;
		}

		auto const& F = nodes[f];
		return visited[f] = count_sets_rec(F.lo, visited) + count_sets_rec(F.hi, visited);
	}

	void count_nodes_rec(node f, std::unordered_set<node>& visited) const
	{
		if (f <= 1 || visited.count(f))
			return;
		visited.insert(f);
		auto const& F = nodes[f];
		count_nodes_rec(F.lo, visited);
		count_nodes_rec(F.hi, visited);
	}

public:
	void ref(node f)
	{
		if (f > 1) {
			nodes[f].ref++;
		}
	}

	void deref(node f)
	{
		if (f > 1 && nodes[f].ref > 0) {
			nodes[f].ref--;
		}
	}

	std::size_t num_nodes() const
	{
		return nodes.size() - 2 - free.size();
	}

	void garbage_collect()
	{
		std::vector<node> to_delete;
		/* skip terminals and elementary nodes */
		for (auto it = nodes.begin() + unique_table.size() + 2; it != nodes.end(); ++it) {
			if (it->ref == 0 && it->dead == 0) {
				to_delete.push_back(std::distance(nodes.begin(), it));
			}
		}
		for (auto f : to_delete) {
			kill_node(f);
			auto& n = nodes[f];
			garbage_collect_rec(n.lo);
			garbage_collect_rec(n.hi);
		}

		/* remove n from compute table */
		for (auto it = compute_table.begin(); it != compute_table.end();) {
			if (nodes[it->second].dead || nodes[std::get<0>(it->first)].dead
			    || nodes[std::get<1>(it->first)].dead) {
				it = compute_table.erase(it);
			} else {
				++it;
			}
		}

		compute3_table.clear(); /* TODO: selective delete? */
	}

private:
	void garbage_collect_rec(node f)
	{
		if (f <= 1)
			return;
		auto& n = nodes[f];
		if (n.ref == 0 || n.dead == 1)
			return;
		if (--(n.ref) == 0) {
			kill_node(f);
			garbage_collect_rec(n.lo);
			garbage_collect_rec(n.hi);
		}
	}

	void kill_node(node f)
	{
		free.push(f);
		auto& n = nodes[f];
		n.dead = 1;

		/* remove n from unique table */
		const auto it = unique_table[n.var].find({(node) n.lo, (node) n.hi});
		assert(it != unique_table[n.var].end());
		assert(it->second == f);
		unique_table[n.var].erase(it);
	}

public:
	struct identity_format {
		constexpr uint32_t operator()(uint32_t v) const noexcept
		{
			return v;
		}
	};

	template<class Fn>
	void foreach_set(node f, Fn&& fn)
	{
		std::vector<uint32_t> set;
		foreach_set_rec(f, set, fn);
	}

	void sets_to_vector(node f, std::vector<std::vector<uint32_t>> &set_vector)
	{
		std::vector<uint32_t> set;
		sets_to_vector_rec(f, set, set_vector);
	}

	template<class Formatter = identity_format>
	void print_sets(node f, Formatter&& fmt = Formatter())
	{
		foreach_set(f, [&](auto const& set) {
			for (auto v : set) {
				std::cout << fmt(v) << " ";
			}
			std::cout << "\n";
			return true;
		});
	}

	template<class Formatter = identity_format>
	void write_dot(std::ostream& os, Formatter&& fmt = Formatter())
	{
		os << "digraph {\n";
		os << "0[shape=rectangle,label=⊥];\n";
		os << "1[shape=rectangle,label=⊤];\n";

		for (auto const& t : unique_table) {
			std::stringstream rank;
			for (auto const& [_, n] : t) {
				auto const& N = nodes[n];
				if (N.dead)
					continue;
				os << n << "[shape=ellipse,label=\"" << fmt(N.var) << "\"];\n";
				os << n << " -> " << N.lo << "[style=dashed]\n";
				os << n << " -> " << N.hi << "\n";
				rank << ";" << n;
			}
			os << "{rank=same" << rank.str() << "}\n";
		}

		os << "}\n";
	}

private:
	template<class Fn>
	bool foreach_set_rec(node f, std::vector<uint32_t>& set, Fn&& fn)
	{
		if (f == 1) {
			return fn(set);
		} else if (f != 0) {
			if (!foreach_set_rec(nodes[f].lo, set, fn)) {
				return false;
			}
			auto set1 = set;
			set1.push_back(nodes[f].var);
			if (!foreach_set_rec(nodes[f].hi, set1, fn)) {
				return false;
			}
		}

		return true;
	}

	void sets_to_vector_rec(node f, std::vector<uint32_t>& set,
	                        std::vector<std::vector<uint32_t>>& set_vector)
	{
		if (f == 1) {
			std::vector<uint32_t> single_set;
			for (auto v : set) {
				single_set.push_back(v);
			}
			set_vector.push_back(single_set);
		} else if (f != 0) {
			sets_to_vector_rec(nodes[f].lo, set, set_vector);
			auto set1 = set;
			set1.push_back(nodes[f].var);
			sets_to_vector_rec(nodes[f].hi, set1, set_vector);
		}
	}

public:
	void debug()
	{
		std::cout << "    i     VAR    LO    HI   REF  DEAD\n";
		int i{0};
		for (auto const& n : nodes) {
			std::cout << std::setw(5) << i++ << " : " << std::setw(5) << n.var << " "
			          << std::setw(5) << n.lo << " " << std::setw(5) << n.hi << " "
			          << std::setw(5) << n.ref << " " << std::setw(5) << n.dead << "\n";
		}
		summary();
	}

	void summary()
	{
		std::cout << "live nodes = " << num_nodes() << "   dead nodes = " << free.size()
		          << "\n";
	}

private: /* hash functions */
	struct unique_table_hash {
		std::size_t operator()(std::pair<uint32_t, uint32_t> const& p) const
		{
			return 12582917 * p.first + 4256249 * p.second;
		}
	};

	struct compute_table_hash {
		std::size_t operator()(std::tuple<uint32_t, uint32_t, op_t> const& p) const
		{
			return 12582917 * std::get<0>(p) + 4256249 * std::get<1>(p)
			       + 741457 * static_cast<uint32_t>(std::get<2>(p));
		}
	};

	struct compute3_table_hash {
		std::size_t operator()(std::tuple<uint32_t, uint32_t, uint32_t, op_t> const& p) const
		{
			return 18803 * std::get<0>(p) + 53777 * std::get<1>(p)
			       + 61231 * std::get<2>(p)
			       + 3571 * static_cast<uint32_t>(std::get<3>(p));
		}
	};

private:
	node unique(uint32_t var, node lo, node hi)
	{
		/* ZDD reduction rule */
		if (hi == 0) {
			return lo;
		}

		assert(nodes[lo].var > var);
		assert(nodes[hi].var > var);

		/* unique table lookup */
		const auto it = unique_table[var].find({lo, hi});
		if (it != unique_table[var].end()) {
			assert(!nodes[it->second].dead);
			return it->second;
		}

		/* create new node */
		node r;

		if (!free.empty()) {
			r = free.top();
			free.pop();
			nodes[r].ref = nodes[r].dead = 0;
			nodes[r] = {var, lo, hi};
		} else if (nodes.size() < nodes.capacity()) {
			r = nodes.size();
			nodes.emplace_back(var, lo, hi);
		} else {
			std::cerr << "[e] no more space for new nodes available\n";
			exit(1);
		}

		/* increase ref counts */
		if (lo > 1) {
			nodes[lo].ref++;
		}
		if (hi > 1) {
			nodes[hi].ref++;
		}

		return unique_table[var][{lo, hi}] = r;
	}

private:
	std::vector<node_t> nodes;
	std::stack<node> free;
	std::vector<std::unordered_map<std::pair<uint32_t, uint32_t>, node, unique_table_hash>> unique_table;
	std::unordered_map<std::tuple<uint32_t, uint32_t, op_t>, node, compute_table_hash> compute_table;
	std::unordered_map<std::tuple<uint32_t, uint32_t, uint32_t, op_t>, node, compute3_table_hash>
	    compute3_table;
};

} // namespace tweedledum
