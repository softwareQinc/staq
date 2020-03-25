/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken, Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include "../../networks/io_id.hpp"
#include "../../utils/angle.hpp"
#include "../../utils/parity_terms.hpp"
#include "gray_synth.hpp"
#include "linear_synth.hpp"

#include <cstdint>
#include <iostream>
#include <easy/esop/constructors.hpp>
#include <easy/esop/esop_from_pkrm.hpp>
#include <easy/esop/esop_from_pprm.hpp>
#include <kitty/kitty.hpp>
#include <vector>

namespace tweedledum {

/*! \brief Parameters for `stg_from_exact_esop`. */
struct stg_from_esop_params {};

/*! \brief Synthesize a quantum network from a function by computing exact ESOP
 * representation
 */
struct stg_from_exact_esop {
    /*! \brief Synthesize into a _existing_ quantum network
     *
     * \param network  A quantum network
     * \param qubits   The subset of qubits the gate acts upon.
     * \param function
     */
    template <class Network>
    void operator()(Network& network, std::vector<io_id> const& qubits,
                    kitty::dynamic_truth_table const& function) const {
        using exact_synthesizer =
            easy::esop::esop_from_tt<kitty::dynamic_truth_table,
                                     easy::sat2::maxsat_rc2,
                                     easy::esop::helliwell_maxsat>;
        const auto num_controls = function.num_vars();
        assert(qubits.size() == static_cast<std::size_t>(num_controls) + 1u);

        easy::esop::helliwell_maxsat_statistics stats;
        easy::esop::helliwell_maxsat_params ps;
        exact_synthesizer synthesizer(stats, ps);

        std::vector<io_id> target = {qubits.back()};
        const auto cubes = synthesizer.synthesize(function);
        for (auto const& cube : cubes) {
            std::vector<io_id> controls;
            std::vector<io_id> negations;
            auto bits = cube._bits;
            auto mask = cube._mask;
            for (auto v = 0; v < num_controls; ++v) {
                if (mask & 1) {
                    controls.emplace_back((bits & 1) ? qubits[v] : !qubits[v]);
                }
                bits >>= 1;
                mask >>= 1;
            }
            network.add_gate(gate::mcx, controls, target);
        }
    }
};

/*! \brief Synthesize a quantum network from a function by computing PKRM
 * representation
 *
 * PKRM: Pseudo-Kronecker Read-Muller expression---a special case of an ESOP
 * form.
 */
struct stg_from_pkrm {
    /*! \brief Synthesize into a _existing_ quantum network
     *
     * \param network  A quantum network
     * \param qubits   The subset of qubits the gate acts upon.
     * \param function
     */
    template <class Network>
    void operator()(Network& network, std::vector<io_id> const& qubits,
                    kitty::dynamic_truth_table const& function) const {
        const auto num_controls = function.num_vars();
        assert(qubits.size() >= static_cast<std::size_t>(num_controls) + 1u);

        std::vector<io_id> target = {qubits.back()};
        for (auto const& cube : easy::esop::esop_from_optimum_pkrm(function)) {
            std::vector<io_id> controls;
            std::vector<io_id> negations;
            auto bits = cube._bits;
            auto mask = cube._mask;
            for (auto v = 0; v < num_controls; ++v) {
                if (mask & 1) {
                    controls.emplace_back((bits & 1) ? qubits[v] : !qubits[v]);
                }
                bits >>= 1;
                mask >>= 1;
            }
            network.add_gate(gate::mcx, controls, target);
        }
    }
};

/*! \brief Synthesize a quantum network from a function by computing PPRM
 * representation
 *
 * PPRM: The positive polarity Reed-Muller form is an ESOP, where each variable
 * has positive polarity (not complemented form). PPRM is a canonical
 * expression, so further minimization is not possible.
 */
struct stg_from_pprm {
    /*! \brief Synthesize into a _existing_ quantum network
     *
     * \param network  A quantum network
     * \param qubits   The subset of qubits the gate acts upon.
     * \param function
     */
    template <class Network>
    void operator()(Network& network, std::vector<io_id> const& qubits,
                    kitty::dynamic_truth_table const& function) const {
        const auto num_controls = function.num_vars();
        assert(qubits.size() >= static_cast<std::size_t>(num_controls) + 1u);

        std::vector<io_id> target = {qubits.back()};
        for (auto const& cube : easy::esop::esop_from_pprm(function)) {
            assert(cube._bits == cube._mask); /* PPRM property */
            std::vector<io_id> controls;
            auto bits = cube._bits;
            for (auto v = 0; v < num_controls; ++v) {
                if (bits & 1) {
                    controls.push_back(qubits[v]);
                }
                bits >>= 1;
            }
            network.add_gate(gate::mcx, controls, target);
        }
    }
};

/*! \brief Parameters for `stg_from_spectrum`. */
struct stg_from_spectrum_params {
    enum class behavior : uint8_t {
        use_linear_synth,
        complete_spectra,
    } behavior = behavior::complete_spectra;

    linear_synth_params ls_params;
    gray_synth_params gs_params;
};

/*! \brief Synthesize a single target gate from a function by computing
 * Rademacher-Walsh spectrum */
struct stg_from_spectrum {
    stg_from_spectrum(stg_from_spectrum_params const& params_ = {})
        : params(params_) {}

    /*! \brief Synthesize a single target gate into a _existing_ quantum network
     *
     * \param network  A quantum network
     * \param qubits   The subset of qubits the gate acts upon.
     * \param function
     */
    template <class Network>
    void operator()(Network& network, std::vector<io_id> const& qubits,
                    kitty::dynamic_truth_table const& function) const {
        const auto num_controls = function.num_vars();
        assert((num_controls + 1u) <= 32u);
        assert(qubits.size() >= num_controls + 1u);

        auto gate_function = kitty::extend_to(function, num_controls + 1);
        auto xt = gate_function.construct();
        kitty::create_nth_var(xt, num_controls);
        gate_function &= xt;

        parity_terms<uint32_t> parities;
        // An angle type create like this is implicitly multiplied by pi
        const angle nom(1, (1 << gate_function.num_vars()));
        const auto spectrum = kitty::rademacher_walsh_spectrum(gate_function);
        for (auto i = 1u; i < spectrum.size(); ++i) {
            if (spectrum[i] == 0) {
                continue;
            }
            parities.add_term(i, nom * spectrum[i]);
        }

        network.add_gate(gate::hadamard, qubits.back());
        if (params.behavior ==
            stg_from_spectrum_params::behavior::use_linear_synth) {
            linear_synth(network, qubits, parities, params.ls_params);
        } else if (parities.num_terms() == spectrum.size() - 1) {
            linear_synth(network, qubits, parities, params.ls_params);
        } else {
            gray_synth(network, qubits, parities, params.gs_params);
        }
        network.add_gate(gate::hadamard, qubits.back());
    }

    stg_from_spectrum_params params;
};

} /* namespace tweedledum */
