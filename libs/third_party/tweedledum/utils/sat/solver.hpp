/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "glucose/glucose.hpp"
#include "types.hpp"

#include <cstdint>
#include <variant>
#include <vector>

namespace tweedledum::sat {

class result {
  public:
    using model_type = std::vector<lbool_type>;
    using clause_type = std::vector<lit_type>;

    enum class states : uint8_t {
        satisfiable,
        unsatisfiable,
        undefined,
        timeout,
    };

    result(states state = states::undefined) : state_(state) {}

    result(model_type const& model)
        : state_(states::satisfiable), data_(model) {}

    result(clause_type const& unsat_core)
        : state_(states::unsatisfiable), data_(unsat_core) {}

#pragma region Properties
    inline bool is_satisfiable() const {
        return (state_ == states::satisfiable);
    }

    inline bool is_unsatisfiable() const {
        return (state_ == states::unsatisfiable);
    }

    inline bool is_undefined() const { return (state_ == states::undefined); }

    inline model_type model() const { return std::get<model_type>(data_); }
#pragma endregion

#pragma region Overloads
    inline operator bool() const { return (state_ == states::satisfiable); }
#pragma endregion

  private:
    states state_;
    std::variant<model_type, clause_type> data_;
};

enum class solvers {
    glucose_41,
};

template <solvers Solver = solvers::glucose_41>
class solver;

template <>
class solver<solvers::glucose_41> {
    using solver_type = Glucose::Solver;

  public:
#pragma region Constructors
    solver() : solver_(std::make_unique<solver_type>()) {}
#pragma endregion

#pragma region Modifiers
    var_type add_variable() { return solver_->newVar(); }

    void add_variables(uint32_t num_variables = 1) {
        for (auto i = 0u; i < num_variables; ++i) {
            solver_->newVar();
        }
    }

    auto add_clause(std::vector<lit_type> const& clause) {
        Glucose::vec<Glucose::Lit> literals;
        for (auto lit : clause) {
            literals.push(
                Glucose::mkLit(lit.variable(), lit.is_complemented()));
        }
        return solver_->addClause_(literals);
    }

    result solve(std::vector<lit_type> const& assumptions = {},
                 uint32_t conflict_limit = 0) {
        assert(solver_->okay() == true);
        if (conflict_limit) {
            solver_->setConfBudget(conflict_limit);
        }

        Glucose::vec<Glucose::Lit> literals;
        for (auto lit : assumptions) {
            literals.push(
                Glucose::mkLit(lit.variable(), lit.is_complemented()));
        }

        Glucose::lbool state = solver_->solveLimited(literals);
        if (state == l_True) {
            result::model_type model;
            for (auto i = 0; i < solver_->model.size(); ++i) {
                if (solver_->model[i] == l_False) {
                    model.emplace_back(lbool_type::false_);
                } else if (solver_->model[i] == l_True) {
                    model.emplace_back(lbool_type::true_);
                } else {
                    model.emplace_back(lbool_type::undefined);
                }
            }
            return result(model);
        } else if (state == l_False) {
            result::clause_type unsat_core;
            for (auto i = 0; i < solver_->conflict.size(); ++i) {
                unsat_core.emplace_back(Glucose::var(solver_->conflict[i]),
                                        Glucose::sign(solver_->conflict[i])
                                            ? negative_polarity
                                            : positive_polarity);
            }
            return result(unsat_core);
        }
        return result();
    }
#pragma endregion

#pragma region Properties
    uint32_t num_variables() const { return solver_->nVars(); }

    uint32_t num_clauses() const { return solver_->nClauses(); }
#pragma endregion

  private:
    std::unique_ptr<solver_type> solver_;
};

} // namespace tweedledum::sat