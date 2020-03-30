/* easy: C++ ESOP library
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <easy/sat2/maxsat.hpp>
#include <easy/sat2/cnf_from_xcnf.hpp>
#include <easy/utils/dynamic_bitset.hpp>

#include <map>
#include <unordered_map>

namespace easy::esop {

namespace detail {

// Added by Vlad on Mar. 30, 2020, Windows compatibility
#ifdef WIN32
int GetLowestBitPos(int value) {
    if (value == 0)
        return 0;
    int pos = 0;
    while (!(value & 1)) {
        value >>= 1;
        ++pos;
    }
    return 1 + pos;
}
#endif // WIN32
// END added by Vlad on Mar. 30, 2020, Windows compatibility

std::vector<uint32_t> compute_flips(uint32_t n) {
    auto const size = (1u << n);
    auto const total_flips = size - 1;
    std::vector<uint32_t> flip_vec(total_flips);

    auto gray_number = 0u;
    auto temp = 0u;
    for (auto i = 1u; i <= total_flips; ++i) {
        gray_number = i ^ (i >> 1);
        flip_vec[total_flips - i] =
// Added by Vlad on Mar. 30, 2020, for Windows compatibility
#ifndef WIN32
            ffs(temp ^ gray_number) - 1u;
#else
            GetLowestBitPos(temp ^ gray_number) - 1u;
#endif  // WIN32
// END added by Vlad on Mar. 30, 2020, Windows compatibility
        temp = gray_number;
    }

    return flip_vec;
}

std::vector<kitty::cube> compute_implicants(const kitty::cube& c,
                                            uint32_t num_vars) {
    const auto flips = compute_flips(num_vars);

    std::vector<kitty::cube> impls = {c};
    auto copy = c;
    for (const auto& flip : flips) {
        if (copy.get_mask(flip)) {
            copy.clear_bit(flip);
            copy.clear_mask(flip);
        } else {
            copy.set_mask(flip);
            if (c.get_bit(flip)) {
                copy.set_bit(flip);
            } else {
                copy.clear_bit(flip);
            }
        }

        impls.emplace_back(copy);
    }

    return impls;
}

struct helliwell_decision_variables {
  public:
    explicit helliwell_decision_variables(int& sid) : _sid(sid) {}

    int lookup_g(const kitty::cube& c) const { return cube_to_g.at(c); }

    const kitty::cube& lookup_cube(int v) const { return g_to_cube.at(v); }

    int operator[](const kitty::cube& c) { return get_or_create_variable(c); }

    std::map<int, kitty::cube>::const_iterator begin() const {
        return g_to_cube.begin();
    }

    std::map<int, kitty::cube>::const_iterator end() const {
        return g_to_cube.end();
    }

    uint64_t size() const { return g_to_cube.size() + 1; }

  protected:
    int get_or_create_variable(const kitty::cube& c) {
        auto it = cube_to_g.find(c);
        if (it == cube_to_g.end()) {
            int variable = _sid++;
            cube_to_g.emplace(c, variable);
            g_to_cube.emplace(variable, c);
            return variable;
        } else {
            return it->second;
        }
    }

  protected:
    int& _sid;

    std::unordered_map<kitty::cube, int, kitty::hash<kitty::cube>> cube_to_g;
    std::map<int, kitty::cube> g_to_cube;
}; /* helliwell_decision_variables */

template <typename TT>
void derive_xor_clauses(std::vector<std::vector<int>>& xor_clauses,
                        helliwell_decision_variables& g, TT const& bits,
                        TT const& care) {
    assert(bits.num_vars() == care.num_vars());

    kitty::cube minterm;
    for (auto i = 0; i < bits.num_vars(); ++i)
        minterm.set_mask(i);

    do {
        if (kitty::get_bit(care, minterm._bits)) {
            std::vector<int> clause;
            for (const auto& impl :
                 compute_implicants(minterm, bits.num_vars())) {
                clause.push_back(g[impl]);
            }

            /* flip the first bit of the xor-clause if the minterm is positive
             */
            if (!kitty::get_bit(bits, minterm._bits)) {
                clause[0u] *= -1;
            }

            xor_clauses.emplace_back(clause);
        }

        ++minterm._bits;
    } while (minterm._bits < (1u << bits.num_vars()));
}

esop_t esop_from_model(sat2::model const& m,
                       helliwell_decision_variables const& g) {
    esop_t esop;
    for (const auto& v : g) {
        if (m[v.first]) {
            esop.emplace_back(v.second);
        }
    }
    return esop;
}

esop_t
esop_from_clause_selectors(std::vector<int> const& sels,
                           helliwell_decision_variables const& g,
                           std::unordered_map<int, int> soft_clause_map) {
    esop_t esop;
    for (const auto& s : sels) {
        esop.push_back(g.lookup_cube(soft_clause_map.at(s)));
    }
    return esop;
}

std::vector<std::vector<int>>
translate_to_cnf(int& sid, std::vector<std::vector<int>> const& xcnf,
                 uint32_t num_vars) {
    return sat2::cnf_from_xcnf(sid, xcnf, num_vars).get();
}

} // namespace detail

struct helliwell_maxsat {};

struct helliwell_maxsat_statistics {};

struct helliwell_maxsat_params {};

template <typename TT, typename Solver>
class esop_from_tt<TT, Solver, helliwell_maxsat> {
  public:
    using maxsat_solver_t = sat2::maxsat_solver<Solver>;

  public:
    explicit esop_from_tt(helliwell_maxsat_statistics& stats,
                          helliwell_maxsat_params& ps)
        : _stats(stats), _ps(ps), _solver(_maxsat_stats, _maxsat_ps, _sid) {}

    /*! \brief Synthesizes an ESOP form from an incompletely-specified Boolean
     * function
     *
     * \param bits Truth table of function
     * \param care Truth table of care function
     */
    esop_t synthesize(TT const& bits, TT const& care,
                      std::function<int(kitty::cube)> const& cost_fn =
                          [](kitty::cube const& cube) { return 1; }) {
        assert(bits.num_vars() == care.num_vars());

        detail::helliwell_decision_variables g(_sid);

        /* derive 2^n constraints in 3^n variables */
        std::vector<std::vector<int>> xor_clauses;
        detail::derive_xor_clauses(xor_clauses, g, bits, care);

        /* apply gause algorithm to translate XOR-clauses to clauses */
        for (const auto& c :
             detail::translate_to_cnf(_sid, xor_clauses, g.size())) {
            _solver.add_clause(c);
        }

        /* add soft clauses and remember how they map onto g */
        std::unordered_map<int, int> soft_clause_map;
        for (const auto& v : g) {
            int cid = _solver.add_soft_clause({-v.first}, cost_fn(v.second));
            soft_clause_map.insert(std::make_pair(cid, v.first));
        }

        /* extract the esop from the model */
        auto const state = _solver.solve();
        if (state == maxsat_solver_t::state::success) {
            auto const clause_selectors = _solver.get_disabled_clauses();
            return detail::esop_from_clause_selectors(clause_selectors, g,
                                                      soft_clause_map);
        } else {
            return {};
        }
    }

    /*! \brief Synthesizes an ESOP form from a completely-specified Boolean
     * function
     *
     * \param bits Truth table of function
     */
    esop_t synthesize(TT const& bits,
                      std::function<int(kitty::cube)> const& cost_fn =
                          [](kitty::cube const& cube) { return 1; }) {
        auto const care = kitty::create<TT>(bits.num_vars());
        return synthesize(bits, ~care, cost_fn);
    }

  protected:
    helliwell_maxsat_statistics& _stats;
    helliwell_maxsat_params const& _ps;

    int _sid = 1;

    sat2::maxsat_solver_statistics _maxsat_stats;
    sat2::maxsat_solver_params _maxsat_ps;
    maxsat_solver_t _solver;
}; /* esop_from_tt */

struct helliwell_sat {};

struct helliwell_sat_statistics {};

struct helliwell_sat_params {};

template <typename TT, typename Solver>
class esop_from_tt<TT, Solver, helliwell_sat> {
  public:
    explicit esop_from_tt(helliwell_sat_statistics& stats,
                          helliwell_sat_params& ps)
        : _stats(stats), _ps(ps), _solver(_sat_stats, _sat_ps) {}

    /*! \brief Synthesizes an ESOP form from an incompletely-specified Boolean
     * function
     *
     * \param bits Truth table of function
     * \param care Truth table of care function
     */
    esop_t synthesize(TT const& bits, TT const& care) {
        assert(bits.num_vars() == care.num_vars());

        detail::helliwell_decision_variables g(_sid);

        /* derive 2^n constraints in 3^n variables */
        std::vector<std::vector<int>> xor_clauses;
        detail::derive_xor_clauses(xor_clauses, g, bits, care);

        /* apply gause algorithm to translate XOR-clauses to clauses */
        for (const auto& c :
             detail::translate_to_cnf(_sid, xor_clauses, g.size())) {
            _solver.add_clause(c);
        }

        /* extract the esop from the model */
        auto const state = _solver.solve();
        if (state == sat2::sat_solver::state::sat) {
            auto const model = _solver.get_model();
            assert(model.size() != 0);
            return detail::esop_from_model(model, g);
        } else {
            return {};
        }
    }

    /*! \brief Synthesizes an ESOP form from a completely-specified Boolean
     * function
     *
     * \param bits Truth table of function
     */
    esop_t synthesize(TT const& bits) {
        auto const care = kitty::create<TT>(bits.num_vars());
        return synthesize(bits, ~care);
    }

  protected:
    helliwell_sat_statistics& _stats;
    helliwell_sat_params const& _ps;

    int _sid = 1;

    sat2::sat_solver_statistics _sat_stats;
    sat2::sat_solver_params _sat_ps;
    sat2::sat_solver _solver;
}; /* esop_from_tt */

} /* namespace easy::esop */

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
