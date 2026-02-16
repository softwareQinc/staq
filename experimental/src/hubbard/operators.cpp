#include <hubbard/operators.hpp>

#include <algorithm>
#include <memory>
#include <set>
#include <sstream>
#include <vector>
#include <hubbard/builders.hpp>
#include <tools_v1/algorithm/LCU.hpp>
#include <tools_v1/algorithm/Multiplication.hpp>
#include <tools_v1/algorithm/QSVT.hpp>

namespace hubbard {

using tools_v1::ast::DeclaredGate;
using tools_v1::ast::VarAccess;
using tools_v1::tools::circuit;
using tools_v1::tools::qbit;

namespace {

circuit make_identity_circuit(std::span<qbit> data) {
  circuit circ;
  if (data.empty()) {
    return circ;
  }
  auto gate = DeclaredGate::create(tools_v1::parser::Position(), "id", {},
                                   {data[0].to_va()});
  circ.push_back(std::move(gate));
  return circ;
}

} // namespace

tools_v1::tools::circuit
build_lcu_A(square_hubbard_config &config, double t, std::span<qbit> data, tools_v1::tools::ANC_MEM &anc_mem, int num_fermions, unsigned ell) {
  double total_energy = 0.0;
  std::vector<double> coeffs;
  coeffs.reserve(num_fermions);
  auto sites = config.decoding_vector();

  for (const auto &[n, coords] : sites) {
    int nx = coords.first;
    int ny = coords.second;
    double en = config.e_bare(nx, ny) + 4.1 * t;
    total_energy += 2 * en;
    coeffs.push_back(en);
    coeffs.push_back(en);
  }

  std::transform(coeffs.begin(), coeffs.end(), coeffs.begin(), [&total_energy](double en) { return en / total_energy; });

  std::vector<qbit> ancilla_lcu_A;
  ancilla_lcu_A.reserve(2 * ell + 1);
  for (unsigned i = 0; i < 2 * ell + 1; ++i) {
    ancilla_lcu_A.emplace_back(anc_mem.generate_ancilla("lcu_A"));
  }

  std::vector<circuit> unitaries_lcu_A;
  unitaries_lcu_A.reserve(num_fermions);
  for (int i = 0; i < num_fermions; ++i) {
    qbit ancilla_number_op = anc_mem.generate_ancilla("number_op");

    std::vector<VarAccess> cx_qarg;
    cx_qarg.push_back(data[i].to_va());
    cx_qarg.push_back(ancilla_number_op.to_va());
    auto x1 =
        DeclaredGate::create(tools_v1::parser::Position(), "x", {},
                             {data[i].to_va()});
    auto cx = DeclaredGate::create(tools_v1::parser::Position(), "cx", {},
                                   std::move(cx_qarg));
    auto x2 =
        DeclaredGate::create(tools_v1::parser::Position(), "x", {},
                             {data[i].to_va()});

    circuit c;
    c.push_back(std::move(x1));
    c.push_back(std::move(cx));
    c.push_back(std::move(x2));
    c.save_ancilla(ancilla_number_op);

    unitaries_lcu_A.push_back(std::move(c));
  }

  return tools_v1::algorithm::LCU(coeffs, ancilla_lcu_A, unitaries_lcu_A);
}

tools_v1::tools::circuit
build_lcu_A_real(square_hubbard_config &config, double t, std::span<qbit> data, tools_v1::tools::ANC_MEM &anc_mem) {
  std::vector<circuit> unitaries_lcu_A;

  int Lmin = config.Lmin();
  int Lmax = config.Lmax();

  auto create_dg = [](circuit &c, const std::string &name, std::vector<VarAccess> &&qargs) {
    auto g = DeclaredGate::create(tools_v1::parser::Position(), name, {}, std::move(qargs));
    c.push_back(std::move(g));
  };

  for (int nx = Lmin; nx <= Lmax; ++nx) {
    for (int ny = Lmin; ny <= Lmax; ++ny) {
      for (unsigned int sg = 0; sg < 2; ++sg) {
        circuit c_ija;

        Ferm_Occ_Idx fi{nx, ny, sg};
        Ferm_Occ_Idx fj{(nx < Lmax ? nx + 1 : Lmin),
                        (ny < Lmax ? ny + 1 : Lmin), sg};

        const int i = config.index_from_occupation(fi);
        const int j = config.index_from_occupation(fj);

        qbit ancilla_number_op = anc_mem.generate_ancilla("number_op");
        c_ija.save_ancilla(ancilla_number_op);

        create_dg(c_ija, "cx", {data[i].to_va(), data[j].to_va()});
        create_dg(c_ija, "x", {data[j].to_va()});
        create_dg(c_ija, "cx", {data[i].to_va(), ancilla_number_op.to_va()});
        create_dg(c_ija, "x", {data[j].to_va()});
        create_dg(c_ija, "cx", {data[i].to_va(), data[j].to_va()});

        int MIN = std::min(i, j);
        int MAX = std::max(i, j);
        create_dg(c_ija, "x", {data[MIN].to_va()});
        for (int k = MIN + 1; k < MAX; ++k) {
          create_dg(c_ija, "z", {data[k].to_va()});
        }
        create_dg(c_ija, "x", {data[MAX].to_va()});
        unitaries_lcu_A.push_back(std::move(c_ija));
      }
    }
  }

  return tools_v1::algorithm::LCU(unitaries_lcu_A, anc_mem);
}

tools_v1::tools::circuit build_B(square_hubbard_config &config,
                                 std::span<qbit> data,
                                 tools_v1::tools::ANC_MEM &anc_mem,
                                 int num_fermions) {
  std::set<std::unique_ptr<qbit>> ancilla_cdag_cdag_c_c;
  auto create_ancilla_cccc = [&]() {
    qbit anc = anc_mem.generate_ancilla("cccc");
    ancilla_cdag_cdag_c_c.insert(std::make_unique<qbit>(anc));
    return std::make_unique<qbit>(anc);
  };

  std::vector<circuit> unitaries_lcu_B;

  auto create_cnot_prep = [&data](circuit &c, const qbit &qc,
                                  const qbit &qt) {
    tools_v1::parser::Position pos;
    std::vector<VarAccess> v;
    v.push_back(qc.to_va());
    v.push_back(qt.to_va());
    auto cnot =
        DeclaredGate::create(pos, "cx", std::vector<tools_v1::ast::ptr<tools_v1::ast::Expr>>{}, std::move(v));
    c.push_back(std::move(cnot));
  };

  auto create_xgate_prep = [&data](circuit &c, const qbit &qc) {
    tools_v1::parser::Position pos;
    std::vector<VarAccess> v;
    v.push_back(qc.to_va());
    auto x = DeclaredGate::create(pos, "x", {}, std::move(v));
    c.push_back(std::move(x));
  };

  auto create_zgate_prep = [&data](circuit &c, const qbit &qc) {
    tools_v1::parser::Position pos;
    std::vector<VarAccess> v;
    v.push_back(qc.to_va());
    auto z = DeclaredGate::create(pos, "z", {}, std::move(v));
    c.push_back(std::move(z));
  };

  auto create_xz_tower = [&data, &create_xgate_prep, &create_zgate_prep](
                             circuit &c, int idx) {
    for (int i = 0; i < idx; ++i) {
      create_zgate_prep(c, data[i]);
    }
    create_xgate_prep(c, data[idx]);
  };

  for (int i = 0; i < num_fermions; i += 2) {
    for (int j = 1; j < num_fermions; j += 2) {
      for (int k = 1; k < num_fermions; k += 2) {
        circuit c_ijk;

        Ferm_Occ_Idx f0 = config.occupation_index(i);
        Ferm_Occ_Idx f1 = config.occupation_index(j);
        Ferm_Occ_Idx f2 = config.occupation_index(k);

        int nx3 = config.brillouin_zone_normalize(f0.nx + f1.nx - f2.nx);
        int ny3 = config.brillouin_zone_normalize(f0.ny + f1.ny - f2.ny);
        Ferm_Occ_Idx f3{nx3, ny3, 0};

        int x0 = i;
        int x1 = j;
        int x2 = k;
        int x3 = config.index_from_occupation(f3);

        if (x0 == x3 && x1 == x2) {
          auto ancilla_op0 = create_ancilla_cccc();
          create_cnot_prep(c_ijk, data[x1], data[x0]);
          create_cnot_prep(c_ijk, data[x0], *ancilla_op0);
          create_cnot_prep(c_ijk, data[x1], data[x0]);
        } else if (x0 != x3 && x1 != x2) {
          auto ancilla_op0 = create_ancilla_cccc();
          auto ancilla_op1 = create_ancilla_cccc();
          auto ancilla_op2 = create_ancilla_cccc();

          create_cnot_prep(c_ijk, data[x1], data[x0]);
          create_cnot_prep(c_ijk, data[x0], *ancilla_op0);
          create_cnot_prep(c_ijk, data[x1], data[x0]);
          create_cnot_prep(c_ijk, data[x2], data[x0]);
          create_xgate_prep(c_ijk, data[x0]);
          create_cnot_prep(c_ijk, data[x0], *ancilla_op1);
          create_xgate_prep(c_ijk, data[x0]);
          create_cnot_prep(c_ijk, data[x2], data[x0]);
          create_cnot_prep(c_ijk, data[x3], data[x0]);
          create_xgate_prep(c_ijk, data[x0]);
          create_cnot_prep(c_ijk, data[x0], *ancilla_op2);
          create_xgate_prep(c_ijk, data[x0]);
          create_cnot_prep(c_ijk, data[x3], data[x0]);
          create_xz_tower(c_ijk, x3);
          create_xz_tower(c_ijk, x2);
          create_xz_tower(c_ijk, x1);
          create_xz_tower(c_ijk, x0);
        } else {
          std::stringstream ss;
          ss << "momentum conservation issue";
          throw ss.str();
        }
        unitaries_lcu_B.emplace_back(std::move(c_ijk));
      }
    }
  }

  circuit lcu_3 = tools_v1::algorithm::LCU(unitaries_lcu_B, anc_mem);
  for (const auto &anc : ancilla_cdag_cdag_c_c) {
    if (anc) {
      lcu_3.save_ancilla(*anc);
    }
  }
  return lcu_3;
}

tools_v1::tools::circuit build_B_real(square_hubbard_config &config,
                                      std::span<qbit> data,
                                      tools_v1::tools::ANC_MEM &anc_mem) {
  std::vector<circuit> unitaries_lcu_B;

  int Lmin = config.Lmin();
  int Lmax = config.Lmax();

  auto create_dg = [](circuit &c, const std::string &name,
                      std::vector<VarAccess> &&qargs) {
    auto g = DeclaredGate::create(tools_v1::parser::Position(), name, {},
                                  std::move(qargs));
    c.push_back(std::move(g));
  };

  for (int nx = Lmin; nx <= Lmax; ++nx) {
    for (int ny = Lmin; ny <= Lmax; ++ny) {
      circuit c_ia;

      Ferm_Occ_Idx fi{nx, ny, 0};

      const int i = config.index_from_occupation(fi);
      const int j = i + 1;

      qbit ancilla_op_0 = anc_mem.generate_ancilla("number_op");
      qbit ancilla_op_1 = anc_mem.generate_ancilla("number_op");
      c_ia.save_ancilla(ancilla_op_0);
      c_ia.save_ancilla(ancilla_op_1);

      create_dg(c_ia, "cx",
                {data[i].to_va(), ancilla_op_0.to_va()});
      create_dg(c_ia, "cx",
                {data[j].to_va(), ancilla_op_1.to_va()});

      unitaries_lcu_B.push_back(std::move(c_ia));
    }
  }

circuit lcu_2 = tools_v1::algorithm::LCU(unitaries_lcu_B, anc_mem);

  return lcu_2;
}

tools_v1::tools::circuit build_ziEA(square_hubbard_config &config,
                                    double t, std::span<qbit> data,
                                    tools_v1::tools::ANC_MEM &anc_mem,
                                    int num_fermions, unsigned ell,
                                    double E0, std::complex<double> z) {
  const std::complex<double> imag{0.0, 1.0};
  circuit id_circuit = make_identity_circuit(data);
  auto lcu_1 =
      build_lcu_A(config, t, data, anc_mem, num_fermions, ell);
  return build_lcu_two_unitaries(z + E0 + imag, -1.0,
                                 std::move(id_circuit), std::move(lcu_1),
                                 anc_mem, "lcu_2");
}

tools_v1::tools::circuit build_ziEA_real(square_hubbard_config &config,
                                         double t, std::span<qbit> data,
                                         tools_v1::tools::ANC_MEM &anc_mem,
                                         double E0, std::complex<double> z) {
  const std::complex<double> imag{0.0, 1.0};
  circuit id_circuit = make_identity_circuit(data);
  auto lcu_1 = build_lcu_A_real(config, t, data, anc_mem);
  return build_lcu_two_unitaries(z + E0 + imag, -t,
                                 std::move(id_circuit), std::move(lcu_1),
                                 anc_mem, "lcu_2");
}

tools_v1::tools::circuit build_ziEA_inverse(square_hubbard_config &config,
                                            double t,
                                            std::span<qbit> data,
                                            tools_v1::tools::ANC_MEM &anc_mem,
                                            int num_fermions, unsigned ell,
                                            double E0,
                                            std::complex<double> z) {
  auto lcu = build_ziEA(config, t, data, anc_mem, num_fermions, ell,
                        E0, z);
  qbit ancilla_qsvt_1 = anc_mem.generate_ancilla("qsvt_1");
  std::vector<double> phi = {0.1777, 0.2777, 0.3777, 0.4777, 0.5777};
  return tools_v1::algorithm::QSVT(phi, lcu, ancilla_qsvt_1);
}

tools_v1::tools::circuit
build_ziEA_inverse_real(square_hubbard_config &config, double t, std::span<qbit> data, tools_v1::tools::ANC_MEM &anc_mem, double E0, std::complex<double> z) {
  auto lcu = build_ziEA_real(config, t, data, anc_mem, E0, z);
  qbit ancilla_qsvt_1 = anc_mem.generate_ancilla("qsvt_1");
  std::vector<double> phi = {0.1777, 0.2777, 0.3777, 0.4777, 0.5777};
  return tools_v1::algorithm::QSVT(phi, lcu, ancilla_qsvt_1);
}

tools_v1::tools::circuit build_iUB(square_hubbard_config &config, std::span<qbit> data, tools_v1::tools::ANC_MEM &anc_mem, int num_fermions) {
  const std::complex<double> imag{0.0, 1.0};
  circuit id_circuit = make_identity_circuit(data);
  auto lcu_1 = build_B(config, data, anc_mem, num_fermions);
  return build_lcu_two_unitaries(imag, -1.0, std::move(id_circuit), std::move(lcu_1), anc_mem, "lcu_iUB");
}

tools_v1::tools::circuit build_iUB_real(square_hubbard_config &config,
                                        std::span<qbit> data,
                                        tools_v1::tools::ANC_MEM &anc_mem) {
  const std::complex<double> imag{0.0, 1.0};
  circuit id_circuit = make_identity_circuit(data);
  auto lcu_1 = build_B_real(config, data, anc_mem);
  return build_lcu_two_unitaries(imag, -1.0, std::move(id_circuit),
                                 std::move(lcu_1), anc_mem, "lcu_iUB");
}

tools_v1::tools::circuit build_I_ziEA_inv_iUB(
    std::span<qbit> data, circuit ziEA_inv, circuit iUB,
    tools_v1::tools::ANC_MEM &anc_mem) {
  const std::complex<double> c0 = 1.0;
  const std::complex<double> c1 = 1.0;
  circuit combined =
      combine_circuits(std::move(ziEA_inv), std::move(iUB));
  circuit id_circuit = make_identity_circuit(data);
  return build_lcu_two_unitaries(c0, c1, std::move(id_circuit),
                                 std::move(combined), anc_mem, "lcu_5");
}

tools_v1::tools::circuit build_AinvB_inverse(
    circuit input, tools_v1::tools::ANC_MEM &anc_mem) {
  qbit ancilla_qsvt_2 = anc_mem.generate_ancilla("qsvt_2");
  std::vector<double> phi2 = {0.1888, 0.2888, 0.3888, 0.4888, 0.5888};
  return tools_v1::algorithm::QSVT(phi2, input, ancilla_qsvt_2);
}

tools_v1::tools::circuit build_observable(
    int creation_index, int annihilation_index, circuit AinvB_inv,
    circuit ziEA_inv, BuildContext &ctx) {
  std::vector<circuit> circuits;
  circuits.emplace_back(build_creation(creation_index, ctx));
  circuits.emplace_back(std::move(AinvB_inv));
  circuits.emplace_back(std::move(ziEA_inv));
  circuits.emplace_back(build_annihilation(annihilation_index, ctx));
  return tools_v1::algorithm::circuit_combine(std::move(circuits));
}

tools_v1::tools::circuit build_lcu_two_unitaries(
    const std::complex<double> &c0, const std::complex<double> &c1,
    circuit id_circuit, circuit target_circuit,
    tools_v1::tools::ANC_MEM &anc_mem, const std::string &ancilla_label) {
  qbit ancilla = anc_mem.generate_ancilla(ancilla_label);
  return tools_v1::algorithm::LCU_two_unitaries(
      c0, c1, id_circuit, target_circuit, ancilla);
}

tools_v1::tools::circuit combine_circuits(circuit lhs, circuit rhs) {
  std::vector<circuit> circuits;
  circuits.emplace_back(std::move(lhs));
  circuits.emplace_back(std::move(rhs));
  tools_v1::tools::circuit out;
  for (auto &u : circuits) {
    for (const auto &gate : u) {
      out.push_back(tools_v1::ast::object::clone(*gate));
    }
    for (auto it = u.ancilla_begin(); it != u.ancilla_end(); ++it) {
      out.save_ancilla(**it);
    }
  }
  return out;
}

} // namespace hubbard
