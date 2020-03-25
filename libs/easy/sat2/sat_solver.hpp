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

/*!
  \file sat_solver.hpp
  \brief SAT-solver interface

  \author Heinz Riener
*/

#pragma once

#ifdef __clang__ // CLANG compiler
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wsign-compare"
#include <glucose/glucose.hpp>
#pragma clang diagnostic pop
#elif __GNUC__ // GCC compiler
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <glucose/glucose.hpp>
#pragma GCC diagnostic pop
#else // other compilers
#include <glucose/glucose.hpp>
#endif

#include <easy/utils/dynamic_bitset.hpp>
#include <algorithm>
#include <iostream>
#include <memory>

namespace easy::sat2 {

/*! \brief Satisfying model (or assignment)
 *
 * A vector of bits obtained from a SAT-solver.  The bit at index 0,
 * 1, 2, and so forth corresponds to the variable with ids 1, 2, 3,
 * and so forth.
 *
 */
class model {
  public:
    /*! \brief Default constructor
     *
     * This constructor creates an empty model.
     */
    explicit model() = default;

    /*! \brief Constructor from dynamic_bitset
     *
     * This constructor creates an empty model.
     */
    explicit model(utils::dynamic_bitset<> const& bs) : _assignment(bs) {}

    /*! \brief Size of model
     *
     * Returns the size of the model.
     */
    uint64_t size() const { return _assignment.num_bits(); }

    /*! \brief Get value of a literal
     *
     * \param lit A literal
     *
     * Returns the value of a literal in the model.
     */
    bool operator[](int lit) const {
        assert(lit != 0);
        uint32_t const var = abs(lit) - 1;
        assert(var < _assignment.num_bits() && "Index out-of-bounds access");
        return lit > 0 ? _assignment[var] : !_assignment[var];
    }

    /*! \brief Print model
     *
     * \param os Output stream
     *
     * Prints the model to os
     */
    void print(std::ostream& os = std::cout) const {
        auto const size = _assignment.num_bits();
        for (auto i = 0u; i < size; ++i) {
            os << _assignment[i];
        }
    }

  protected:
    utils::dynamic_bitset<> _assignment;
}; /* model */

/*! \brief Unsatisfiable core
 *
 * A sorted vector of assumption literals that together with the hard
 * clauses of the SAT-solver cannot be satisfied.
 *
 * In general, the core is not minimal.
 */
template <typename SorterPred = std::less<int>>
class core {
  public:
    /*! \brief Default constructor
     *
     * This constructor creates an empty core.
     */
    explicit core() = default;

    /*! \brief Constructs a core from a vector of literals.
     *
     * This constructor creates a (sorted) core from a vector of literals.
     *
     * \param conflicts A vector of conflict literals.
     */
    explicit core(std::vector<int> const& conflict) : _conflict(conflict) {
        sort(SorterPred());
    }

    /*! \brief Cast operator to std::vector<int>.
     *
     * This automatic cast operator allows an unsatisfiable core to be
     * converted to vector of assumptions.
     *
     */
    operator std::vector<int>() const { return _conflict; }

    /*! \brief Size of core
     *
     * Returns the size of the core.
     */
    uint64_t size() const { return _conflict.size(); }

    /*! \brief Get assumption in core
     *
     * \param pos A position
     *
     * Returns an assumption in the core
     */
    int operator[](uint32_t pos) const {
        assert(pos < _conflict.size());
        return _conflict[pos];
    }

    /*! \brief Print core
     *
     * \param os Output stream
     *
     * Prints the core to os
     */
    void print(std::ostream& os = std::cout) const {
        auto const size = _conflict.size();
        for (auto i = 0u; i < size; ++i) {
            os << _conflict[i] << ' ';
        }
    }

  protected:
    template <typename Pred>
    void sort(Pred&& pred) {
        std::sort(std::begin(_conflict), std::end(_conflict), pred);
    }

  protected:
    std::vector<int> _conflict;
}; /* core */

struct sat_solver_statistics {};

struct sat_solver_params {
    mutable int64_t budget{-1}; /*>! Conflict budget (a value < 0 denotes an
                                   unconstrained budget) */
};

class sat_solver {
  public:
    enum class state {
        fresh = 0,
        dirty = 1,
        sat = 2,
        unsat = 3,
    }; /* state */

  public:
    /* \brief Constructor
     *
     * Constructs a SAT-solver
     *
     * \param stats Statistics
     * \param ps Parameters
     */
    explicit sat_solver(sat_solver_statistics& stats, sat_solver_params& ps)
        : _glucose(std::make_unique<Glucose::Solver>()), _stats(stats),
          _ps(ps) {}

    /* \brief Set conflict budget */
    void set_budget(int64_t budget) {
        _ps.budget = budget;
        _glucose->setConfBudget(budget);
    }

    /* \brief Reset conflict budget */
    void reset_budget() {
        _ps.budget = -1;
        _glucose->budgetOff();
    }

    /*! \brief Return the current state of the SAT-solver */
    state get_state() const { return _state; }

    /*! \brief Returns the number of variables */
    uint32_t get_num_variables() const { return _num_variables; }

    /*! \brief Check satisfiability under assumptions with respect to the
     * conflict budget
     *
     * \param assumption A vector of assumption literals assumed to be true
     *
     * Returns the current state of the SAT-solver
     */
    state solve(std::vector<int> const& assumptions = {}) {
        if (_ps.budget > -1) {
            return solve_limited(assumptions);
        } else {
            return solve_unlimited(assumptions);
        }
    }

    /*! \brief Check satisfiability under assumptions without considering the
     * conflict budget
     *
     * \param assumption A vector of assumption literals assumed to be true
     *
     * Returns the current state of the SAT-solver
     */
    state solve_unlimited(std::vector<int> const& assumptions = {}) {
        _glucose->budgetOff();

        Glucose::vec<Glucose::Lit> ass;
        if (assumptions.size() > 0) {
            for (const auto& l : assumptions) {
                const uint32_t v = abs(l) - 1;
                while (_num_variables <= v) {
                    _glucose->newVar();
                    ++_num_variables;
                }
                ass.push(Glucose::mkLit(v, l < 0));
            }
        }

        auto const result = _glucose->solve(ass);
        if (result) {
            return (_state = state::sat);
        } else {
            return (_state = state::unsat);
        }
    }

    /*! \brief Check satisfiability under assumptions with respect to the
     * conflict budget
     *
     * \param assumption A vector of assumption literals assumed to be true
     *
     * Returns the current state of the SAT-solver
     */
    state solve_limited(std::vector<int> const& assumptions = {}) {
        Glucose::vec<Glucose::Lit> ass;
        if (assumptions.size() > 0) {
            for (const auto& l : assumptions) {
                const uint32_t v = abs(l) - 1;
                while (_num_variables <= v) {
                    _glucose->newVar();
                    ++_num_variables;
                }
                ass.push(Glucose::mkLit(v, l < 0));
            }
        }

        auto const result = _glucose->solveLimited(ass);
        if (result == Glucose::l_Undef ||
            int32_t(_glucose->conflicts) >= _ps.budget) {
            return (_state = state::dirty);
        } else if (result == Glucose::l_True) {
            return (_state = state::sat);
        } else {
            assert(result == Glucose::l_False);
            return (_state = state::unsat);
        }
    }

    /*! \brief Add a clause to the SAT-solver */
    void add_clause(std::vector<int> const& clause) {
        /* update state */
        _state = state::dirty;

        Glucose::vec<Glucose::Lit> cl;
        for (const auto& l : clause) {
            const uint32_t v = abs(l) - 1;
            while (_num_variables <= v) {
                _glucose->newVar();
                ++_num_variables;
            }
            cl.push(Glucose::mkLit(v, l < 0));
        }
        _glucose->addClause(cl);
    }

    /*! \brief Returns model if solver is in state SAT */
    model get_model() const {
        assert(is_sat());

        const uint32_t size = _glucose->model.size();
        utils::dynamic_bitset<> m;
        for (auto i = 0u; i < size; ++i) {
            m.push_back(_glucose->model[i] == Glucose::l_True);
        }
        return model(m);
    }

    /*! \brief Return core if solver is in UNSAT state */
    core<> get_core() const {
        assert(is_unsat());

        const uint32_t size = _glucose->conflict.size();
        std::vector<int> lits(size);
        for (auto i = 0u; i < size; ++i) {
            lits[i] = Glucose::sign(_glucose->conflict[i])
                          ? ((Glucose::var(_glucose->conflict[i]) + 1))
                          : -(Glucose::var(_glucose->conflict[i]) + 1);
        }
        return core(lits);
    }

    /*! \brief Returns true if and only if SAT-solver is in state UNKNOWN. */
    bool is_unknown() const { return _state == state::dirty; }

    /*! \brief Returns true if and only if SAT-solver state is in state SAT. */
    bool is_sat() const { return _state == state::sat; }

    /*! \brief Returns true if and only if SAT-solver is in state UNSAT. */
    bool is_unsat() const { return _state == state::unsat; }

  protected:
    std::unique_ptr<Glucose::Solver> _glucose;
    sat_solver_statistics& _stats;
    sat_solver_params const& _ps;
    state _state{state::fresh};
    uint32_t _num_variables{0};
}; /* sat_solver */

} /* namespace easy::sat2 */
