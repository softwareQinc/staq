/*------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Giulia Meuli, Mathias Soeken
*-----------------------------------------------------------------------------*/
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include <fmt/format.h>
#include <mockturtle/utils/stopwatch.hpp>
#include <percy/solvers/bsat2.hpp>
#include <tweedledum/gates/gate_base.hpp>
#include <tweedledum/networks/qubit.hpp>
#include <tweedledum/utils/bit_matrix_rm.hpp>
#include <tweedledum/utils/parity_terms.hpp>

namespace caterpillar {

using namespace tweedledum;

struct satbased_cnotrz_params {
    /* \brief SAT solver conflict limit. */
    int conflict_limit{0};

    /*! \brief Be verbose. */
    bool verbose{false};
};

struct satbased_cnotrz_stats {
    /*! \brief Total runtime */
    mockturtle::stopwatch<>::duration time_total{};

    /*! \brief Solving time */
    mockturtle::stopwatch<>::duration time_solving{};

    void report() {
        std::cout << fmt::format("[i] time (SAT solving) = {:7.2f} secs\n",
                                 mockturtle::to_seconds(time_solving));
        std::cout << fmt::format("[i] time (total)       = {:7.2f} secs\n",
                                 mockturtle::to_seconds(time_total));
    }
};

namespace detail {

template <class Network>
class satbased_cnotrz_impl {
  public:
    satbased_cnotrz_impl(bit_matrix_rm<> const& transform,
                         parity_terms const& parities,
                         satbased_cnotrz_params const& ps,
                         satbased_cnotrz_stats& st)
        : num_qubits(transform.num_rows()), transform(transform),
          parities(parities), ps(ps), st(st),
          offset(num_qubits * num_qubits + 2 * num_qubits +
                 parities.num_terms() * (num_qubits + 1)) {}

    Network run() {
        mockturtle::stopwatch<> t(st.time_total);

        /* initial state */
        assert_initial();

        auto time = 0u;
        while (true) {
            if (ps.verbose) {
                std::cout << "try with " << time << " steps\n";
            }

            assert_hit_terms(time);
            assert_symmetry_break_matrix(time);

            auto assumps = assume_final(time);
            const auto result =
                mockturtle::call_with_stopwatch(st.time_solving, [&]() {
                    return solver.solve(&assumps[0],
                                        &assumps[0] + assumps.size(),
                                        ps.conflict_limit);
                });
            if (result == percy::synth_result::success) {
                return extract_solution(time);
            }

            /* add new time step */
            assert_control_and_target(time);
            assert_transition(time);

            if (time > 0) {
                assert_symmetry_break(time);
            }

            ++time;
        }

        Network netlist;
        return netlist;
    }

  private:
    void assert_initial() {
        for (auto row = 0u; row < num_qubits; ++row) {
            for (auto column = 0u; column < num_qubits; ++column) {
                int lit = pabc::Abc_Var2Lit(matrix_var(0, row, column),
                                            row != column);
                solver.add_clause(&lit, &lit + 1);
            }
        }
    }

    std::vector<int> assume_final(uint32_t time) {
        std::vector<int> assumptions;

        for (auto row = 0u; row < num_qubits; ++row) {
            for (auto column = 0u; column < num_qubits; ++column) {
                assumptions.push_back(
                    pabc::Abc_Var2Lit(matrix_var(time, row, column),
                                      1 - transform.at(row, column)));
            }
        }

        for (auto id = 0u; id < parities.num_terms(); ++id) {
            assumptions.push_back(
                pabc::Abc_Var2Lit(parity_term_var(time, id), 0));
        }
        return assumptions;
    }

    void assert_control_and_target(uint32_t time) {
        /* one hot condition */
        std::vector<int> control(num_qubits), target(num_qubits);
        for (auto row = 0u; row < num_qubits; ++row) {
            control[row] = pabc::Abc_Var2Lit(control_var(time, row), 0);
            target[row] = pabc::Abc_Var2Lit(target_var(time, row), 0);
        }

        solver.add_clause(&control[0], &control[0] + num_qubits);
        solver.add_clause(&target[0], &target[0] + num_qubits);

        int clause[2];
        for (auto j = 1u; j < num_qubits; ++j) {
            for (auto i = 0u; i < j; ++i) {
                clause[0] = pabc::Abc_Var2Lit(control_var(time, i), 1);
                clause[1] = pabc::Abc_Var2Lit(control_var(time, j), 1);
                solver.add_clause(clause, clause + 2);

                clause[0] = pabc::Abc_Var2Lit(target_var(time, i), 1);
                clause[1] = pabc::Abc_Var2Lit(target_var(time, j), 1);
                solver.add_clause(clause, clause + 2);
            }
        }

        /* control and target cannot both be on the same row */
        for (auto row = 0u; row < num_qubits; ++row) {
            clause[0] = pabc::Abc_Var2Lit(control_var(time, row), 1);
            clause[1] = pabc::Abc_Var2Lit(target_var(time, row), 1);
            solver.add_clause(clause, clause + 2);
        }
    }

    void assert_transition(uint32_t time) {
        int clause[5];

        for (auto row = 0u; row < num_qubits; ++row) {
            for (auto column = 0u; column < num_qubits; ++column) {
                /* if target != row, nothing changes */
                clause[0] = pabc::Abc_Var2Lit(target_var(time, row), 0);
                clause[1] = pabc::Abc_Var2Lit(matrix_var(time, row, column), 1);
                clause[2] =
                    pabc::Abc_Var2Lit(matrix_var(time + 1, row, column), 0);
                solver.add_clause(clause, clause + 3);

                clause[1] = pabc::Abc_Var2Lit(matrix_var(time, row, column), 0);
                clause[2] =
                    pabc::Abc_Var2Lit(matrix_var(time + 1, row, column), 1);
                solver.add_clause(clause, clause + 3);

                for (auto rr = 0u; rr < num_qubits; ++rr) {
                    if (rr == row)
                        continue;

                    clause[0] = pabc::Abc_Var2Lit(target_var(time, row), 1);
                    clause[1] = pabc::Abc_Var2Lit(control_var(time, rr), 1);
                    clause[2] =
                        pabc::Abc_Var2Lit(matrix_var(time, row, column), 1);
                    clause[3] =
                        pabc::Abc_Var2Lit(matrix_var(time, rr, column), 1);
                    clause[4] =
                        pabc::Abc_Var2Lit(matrix_var(time + 1, row, column), 1);
                    solver.add_clause(clause, clause + 5);

                    clause[2] =
                        pabc::Abc_Var2Lit(matrix_var(time, row, column), 1);
                    clause[3] =
                        pabc::Abc_Var2Lit(matrix_var(time, rr, column), 0);
                    clause[4] =
                        pabc::Abc_Var2Lit(matrix_var(time + 1, row, column), 0);
                    solver.add_clause(clause, clause + 5);

                    clause[2] =
                        pabc::Abc_Var2Lit(matrix_var(time, row, column), 0);
                    clause[3] =
                        pabc::Abc_Var2Lit(matrix_var(time, rr, column), 1);
                    clause[4] =
                        pabc::Abc_Var2Lit(matrix_var(time + 1, row, column), 0);
                    solver.add_clause(clause, clause + 5);

                    clause[2] =
                        pabc::Abc_Var2Lit(matrix_var(time, row, column), 0);
                    clause[3] =
                        pabc::Abc_Var2Lit(matrix_var(time, rr, column), 0);
                    clause[4] =
                        pabc::Abc_Var2Lit(matrix_var(time + 1, row, column), 1);
                    solver.add_clause(clause, clause + 5);
                }
            }
        }
    }

    void assert_symmetry_break_matrix(uint32_t time) {
        std::vector<int> clause(num_qubits);

        // NOTE
        /* With both symmetry breaks enabled, the runtime gets worse. With only
           one break enabled, runtime improves, slightly better with the second
           one. For larger instances it may be beneficial to have both breaks in
           place.
        */

        /* there cannot be a row with all zeroes */
        /*for ( auto row = 0u; row < num_qubits; ++row )
        {
          for ( auto column = 0u; column < num_qubits; ++column )
          {
            clause[column] = pabc::Abc_Var2Lit( matrix_var( time, row, column ),
        0 );
          }
          solver.add_clause( &clause[0], &clause[0] + num_qubits );
        }*/

        /* there cannot be a column with all zeroes */
        for (auto column = 0u; column < num_qubits; ++column) {
            for (auto row = 0u; row < num_qubits; ++row) {
                clause[row] =
                    pabc::Abc_Var2Lit(matrix_var(time, row, column), 0);
            }
            solver.add_clause(&clause[0], &clause[0] + num_qubits);
        }
    }

    void assert_symmetry_break(uint32_t time) {
        int clause[4];

        /* same control, first target must be smaller than others */
        for (auto row = 0u; row < num_qubits; ++row) {
            for (auto rr2 = 1u; rr2 < num_qubits; ++rr2) {
                if (rr2 == row)
                    continue;

                for (auto rr1 = 0u; rr1 < rr2; ++rr1) {
                    if (rr1 == row)
                        continue;

                    clause[0] =
                        pabc::Abc_Var2Lit(control_var(time - 1, row), 1);
                    clause[1] = pabc::Abc_Var2Lit(control_var(time, row), 1);
                    clause[2] = pabc::Abc_Var2Lit(target_var(time - 1, rr2), 1);
                    clause[3] = pabc::Abc_Var2Lit(target_var(time, rr1), 1);
                    solver.add_clause(clause, clause + 4);
                }
            }
        }

        /* first control can only be larger if targets are blocking (doesn't
         * work) */
        // for ( auto rr2 = 1u; rr2 < num_qubits; ++rr2 )
        //{
        //  for ( auto rr1 = 0u; rr1 < rr2; ++rr1 )
        //  {
        //    clause[0] = pabc::Abc_Var2Lit( control_var( time - 1, rr2 ), 1 );
        //    clause[1] = pabc::Abc_Var2Lit( control_var( time, rr1 ), 1 );
        //    clause[2] = pabc::Abc_Var2Lit( target_var( time - 1, rr1 ), 0 );
        //    clause[3] = pabc::Abc_Var2Lit( target_var( time, rr2 ), 0 );
        //    solver.add_clause( clause, clause + 4 );
        //  }
        //}
    }

    void assert_and(std::vector<int> const& lits, int output) {
        std::vector<int> lclause(lits.size() + 1u);
        int sclause[2];

        for (auto i = 0u; i < lits.size(); ++i) {
            sclause[0] = lits[i];
            sclause[1] = pabc::Abc_Var2Lit(output, 1);
            solver.add_clause(sclause, sclause + 2);
            lclause[i] = pabc::Abc_LitNot(lits[i]);
        }
        lclause.back() = pabc::Abc_Var2Lit(output, 0);
        solver.add_clause(&lclause[0], &lclause[0] + lclause.size());
    }

    void assert_or(std::vector<int> const& lits, int output) {
        std::vector<int> lclause(lits.size() + 1u);
        int sclause[2];

        for (auto i = 0u; i < lits.size(); ++i) {
            sclause[0] = pabc::Abc_LitNot(lits[i]);
            sclause[1] = pabc::Abc_Var2Lit(output, 0);
            solver.add_clause(sclause, sclause + 2);
            lclause[i] = lits[i];
        }
        lclause.back() = pabc::Abc_Var2Lit(output, 1);
        solver.add_clause(&lclause[0], &lclause[0] + lclause.size());
    }

    void assert_hit_terms(uint32_t time) {
        std::vector<int> and_inputs(num_qubits);
        std::vector<int> or_inputs(num_qubits + (time > 0 ? 1 : 0));
        int clause[2];

        auto ctr = 0u;
        for (auto const& [term, _] : parities) {
            /* AND for each row into row_parity vars */
            for (auto row = 0u; row < num_qubits; ++row) {
                for (auto column = 0u; column < num_qubits; ++column) {
                    const auto pol = (term >> column) & 1;
                    and_inputs[column] = pabc::Abc_Var2Lit(
                        matrix_var(time, row, column), 1 - pol);
                }
                assert_and(and_inputs, parity_term_row_var(time, ctr, row));
                or_inputs[row] =
                    pabc::Abc_Var2Lit(parity_term_row_var(time, ctr, row), 0);
            }

            /* OR of all parity vars */
            if (time > 0) {
                or_inputs[num_qubits] =
                    pabc::Abc_Var2Lit(parity_term_var(time - 1, ctr), 0);

                /* propagate parity term solution */
                clause[0] =
                    pabc::Abc_Var2Lit(parity_term_var(time - 1, ctr), 1);
                clause[1] = pabc::Abc_Var2Lit(parity_term_var(time, ctr), 0);
                solver.add_clause(clause, clause + 2);
            }
            assert_or(or_inputs, parity_term_var(time, ctr));
            ++ctr;
        }
    }

    Network extract_solution(uint32_t time) {
        Network netlist;

        std::vector<uint32_t> qubits_states;
        for (auto i = 0u; i < num_qubits; ++i) {
            netlist.add_qubit();
            qubits_states.emplace_back((1u << i));

            if (auto rotation = parities.extract_term(qubits_states[i]);
                rotation != 0.0) {
                netlist.add_gate(gate_base(gate_set::rotation_z, rotation), i);
            }
        }

        for (auto i = 0u; i < time; ++i) {
            uint32_t c = 0u, t = 0u;
            for (auto row = 0u; row < num_qubits; ++row) {
                if (solver.var_value(control_var(i, row))) {
                    c = row;
                } else if (solver.var_value(target_var(i, row))) {
                    t = row;
                }
            }
            netlist.add_gate(gate::cx, c, t);
            qubits_states[t] ^= qubits_states[c];
            if (auto rotation = parities.extract_term(qubits_states[t]);
                rotation != 0.0) {
                netlist.add_gate(gate_base(gate_set::rotation_z, rotation), t);
            }
        }

        return netlist;
    }

  private:
    inline int matrix_var(uint32_t time, uint32_t row, uint32_t column) const {
        return time * offset + row * num_qubits + column;
    }

    inline int control_var(uint32_t time, uint32_t row) const {
        return time * offset + num_qubits * num_qubits + row;
    }

    inline int target_var(uint32_t time, uint32_t row) const {
        return time * offset + num_qubits * num_qubits + num_qubits + row;
    }

    inline int parity_term_var(uint32_t time, uint32_t id) const {
        return time * offset + num_qubits * num_qubits + 2 * num_qubits +
               id * (num_qubits + 1);
    }

    inline int parity_term_row_var(uint32_t time, uint32_t id,
                                   uint32_t row) const {
        return time * offset + num_qubits * num_qubits + 2 * num_qubits +
               id * (num_qubits + 1) + 1 + row;
    }

  private:
    uint32_t num_qubits;
    bit_matrix_rm<> const& transform;
    parity_terms parities;
    satbased_cnotrz_params const& ps;
    satbased_cnotrz_stats& st;

    uint32_t offset;

    percy::bsat_wrapper solver;
};

} // namespace detail

template <class Network>
Network satbased_cnotrz(bit_matrix_rm<> const& transform,
                        parity_terms const& parities,
                        satbased_cnotrz_params const& ps = {}) {
    satbased_cnotrz_stats st;
    detail::satbased_cnotrz_impl<Network> impl(transform, parities, ps, st);

    const auto result = impl.run();

    if (ps.verbose) {
        st.report();
    }

    return result;
}

} // namespace caterpillar
