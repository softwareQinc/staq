#ifndef HUBBARD_OPERATORS_HPP_
#define HUBBARD_OPERATORS_HPP_

#include <complex>
#include <span>
#include <string>
#include <vector>
#include <square_hubbard_config.hpp>
#include <tools_v1/ast/program.hpp>
#include <tools_v1/ast/stmt.hpp>
#include <tools_v1/tools/ancilla_management.hpp>
#include <tools_v1/tools/staq_builder.hpp>

namespace hubbard {

struct BuildContext;

tools_v1::tools::circuit build_lcu_A(square_hubbard_config &config,
                                     double t,
                                     std::span<tools_v1::tools::qbit> data,
                                     tools_v1::tools::ANC_MEM &anc_mem,
                                     int num_fermions, unsigned ell);

tools_v1::tools::circuit build_lcu_A_real(square_hubbard_config &config,
                                          double t,
                                          std::span<tools_v1::tools::qbit> data,
                                          tools_v1::tools::ANC_MEM &anc_mem);

tools_v1::tools::circuit build_B(square_hubbard_config &config,
                                 std::span<tools_v1::tools::qbit> data,
                                 tools_v1::tools::ANC_MEM &anc_mem,
                                 int num_fermions);

tools_v1::tools::circuit build_B_real(square_hubbard_config &config,
                                     std::span<tools_v1::tools::qbit> data,
                                     tools_v1::tools::ANC_MEM &anc_mem);

tools_v1::tools::circuit build_ziEA(square_hubbard_config &config,
                                    double t,
                                    std::span<tools_v1::tools::qbit> data,
                                    tools_v1::tools::ANC_MEM &anc_mem,
                                    int num_fermions, unsigned ell,
                                    double E0, std::complex<double> z);

tools_v1::tools::circuit build_ziEA_real(square_hubbard_config &config,
                                         double t,
                                         std::span<tools_v1::tools::qbit> data,
                                         tools_v1::tools::ANC_MEM &anc_mem,
                                         double E0, std::complex<double> z);

tools_v1::tools::circuit build_ziEA_inverse(square_hubbard_config &config,
                                            double t,
                                            std::span<tools_v1::tools::qbit> data,
                                            tools_v1::tools::ANC_MEM &anc_mem,
                                            int num_fermions, unsigned ell,
                                            double E0, std::complex<double> z);

tools_v1::tools::circuit build_ziEA_inverse_real(
    square_hubbard_config &config, double t,
    std::span<tools_v1::tools::qbit> data,
    tools_v1::tools::ANC_MEM &anc_mem, double E0,
    std::complex<double> z);

tools_v1::tools::circuit build_iUB(square_hubbard_config &config,
                                   std::span<tools_v1::tools::qbit> data,
                                   tools_v1::tools::ANC_MEM &anc_mem,
                                   int num_fermions);

tools_v1::tools::circuit build_iUB_real(square_hubbard_config &config,
                                        std::span<tools_v1::tools::qbit> data,
                                        tools_v1::tools::ANC_MEM &anc_mem);

tools_v1::tools::circuit build_I_ziEA_inv_iUB(
    std::span<tools_v1::tools::qbit> data,
    tools_v1::tools::circuit ziEA_inv,
    tools_v1::tools::circuit iUB,
    tools_v1::tools::ANC_MEM &anc_mem);

tools_v1::tools::circuit build_AinvB_inverse(
    tools_v1::tools::circuit input,
    tools_v1::tools::ANC_MEM &anc_mem);

tools_v1::tools::circuit build_observable(
    int creation_index, int annihilation_index,
    tools_v1::tools::circuit AinvB_inv,
    tools_v1::tools::circuit ziEA_inv,
    BuildContext &ctx);

tools_v1::tools::circuit build_lcu_two_unitaries(
    const std::complex<double> &c0, const std::complex<double> &c1,
    tools_v1::tools::circuit id_circuit,
    tools_v1::tools::circuit target_circuit,
    tools_v1::tools::ANC_MEM &anc_mem,
    const std::string &ancilla_label);

tools_v1::tools::circuit combine_circuits(
    tools_v1::tools::circuit lhs, tools_v1::tools::circuit rhs);

} // namespace hubbard

#endif // HUBBARD_OPERATORS_HPP_
