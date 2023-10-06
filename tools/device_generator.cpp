/*
 * This file is part of staq.
 *
 * Copyright (c) 2019 - 2023 softwareQ Inc. All rights reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <tuple>

#include <CLI/CLI.hpp>

#include "mapping/device.hpp"

static double FIDELITY_1 = staq::mapping::FIDELITY_1;

void write_to_stream(const std::vector<std::vector<bool>>& adj,
                     const std::string& device_name, std::ostream& out) {
    using Device = staq::mapping::Device;
    Device dev(device_name, static_cast<int>(adj.size()), adj);
    out << dev.to_json() << "\n";
}

void add_edge(std::vector<std::vector<bool>>& adj,
              std::vector<std::vector<double>>& tq_fi, int control, int target,
              double fidelity = FIDELITY_1) {
    if (control < 0 || control >= adj.size() || target < 0 ||
        target >= adj.size())
        std::cerr << "Qubit(s) out of range: " << control << "," << target
                  << "\n";
    else {
        adj[control][target] = true;
        if (fidelity != FIDELITY_1) {
            if (fidelity < 0 || fidelity > 1)
                std::cerr << "Fidelity out of range: " << fidelity << "\n";
            else
                tq_fi[control][target] = fidelity;
        }
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << "Usage: staq_device_generator [OPTIONS] [SUBCOMMAND]\n"
                  << "Run with --help for more information.\n";
        return 0;
    }

    std::vector<int> rectangular; // dimensions for rectangular QPU
    int circular = 0;             // qubit count for circular QPU
    int linear = 0;               // qubit count for linear QPU

    int qubits = 0;                             // qubit count for graph
    std::vector<std::pair<int, double>> fidels; // single qubit fidelities
    std::vector<std::pair<int, int>> d_edges;   // directed edges
    std::vector<std::pair<int, int>> u_edges;   // undirected edges
    std::vector<std::tuple<int, int, double>>
        df_edges; // d-edges with fidelities
    std::vector<std::tuple<int, int, double>>
        uf_edges;                       // u-edges with fidelities
    std::string name = "Custom device"; // name of custom device

    CLI::App app{"Device JSON generator"};
    app.get_formatter()->column_width(43);

    CLI::Option_group* ogroup =
        app.add_option_group("Layout", "Device qubit layout");
    ogroup
        ->add_option("-r,--rectangle", rectangular,
                     "Rectangular QPU dimensions (e.g. -r 3 4) (>= 2)")
        ->expected(1, 2);
    ogroup->add_option("-c,--circle", circular,
                       "Circular QPU qubit count (>= 3)");
    ogroup->add_option("-l,--line", linear, "Linear QPU qubit count (>= 2)");
    ogroup->require_option(0, 1);

    /* Allow user to specify qubits, couplings, and fidelities */
    CLI::App* graph = app.add_subcommand("graph", "Customized device");
    graph->get_formatter()->label("REQUIRED", "(REQUIRED)");
    graph->add_option("-n,--qubits", qubits, "Number of qubits")->required();
    graph->add_option("--name", name, "Device name");
    graph->add_option("-f,--fidelity", fidels, "Single qubit fidelity")
        ->take_all();
    graph->add_option("-d,--directed", d_edges, "Directed edge")->take_all();
    graph
        ->add_option("-D,--directed-f", df_edges, "Directed edge with fidelity")
        ->take_all();
    graph->add_option("-u,--undirected", u_edges, "Undirected edge")
        ->take_all();
    graph
        ->add_option("-U,--undirected-f", uf_edges,
                     "Undirected edge with fidelity")
        ->take_all();

    CLI11_PARSE(app, argc, argv);

    if (*graph) {
        if (qubits > 0) {
            int n = qubits;
            // compute adjacency matrix
            std::vector<double> sq_fi(n, FIDELITY_1);
            std::vector<std::vector<bool>> adj(n, std::vector<bool>(n));
            std::vector<std::vector<double>> tq_fi(
                n, std::vector<double>(n, FIDELITY_1));

            for (auto& x : fidels) {
                if (x.first < 0 || x.first >= n)
                    std::cerr << "Qubit out of range: " << x.first;
                else if (x.second < 0 || x.second > 1)
                    std::cerr << "Fidelity out of range: " << x.second;
                else
                    sq_fi[x.first] = x.second;
            }
            for (auto& x : d_edges)
                add_edge(adj, tq_fi, x.first, x.second);
            for (auto& x : df_edges)
                add_edge(adj, tq_fi, std::get<0>(x), std::get<1>(x),
                         std::get<2>(x));
            for (auto& x : u_edges) {
                add_edge(adj, tq_fi, x.first, x.second);
                add_edge(adj, tq_fi, x.second, x.first);
            }
            for (auto& x : uf_edges) {
                add_edge(adj, tq_fi, std::get<0>(x), std::get<1>(x),
                         std::get<2>(x));
                add_edge(adj, tq_fi, std::get<1>(x), std::get<0>(x),
                         std::get<2>(x));
            }

            staq::mapping::Device dev(name, n, adj, sq_fi, tq_fi);
            std::cout << dev.to_json() << "\n";
        }
    } else if (!rectangular.empty()) {
        int l, w;
        l = rectangular[0];
        if (rectangular.size() >= 2) {
            w = rectangular[1];
        } else {
            w = l;
        }
        if (l >= 2 && w >= 2) {
            /** Qubits are arranged as follows:
             *    0           1        ...     l-1
             *    l          l+1       ...    2l-1
             *    .           .                 .
             *    :           :                 :
             * l*(w-1)    l*(w-1)+1    ...    l*w-1
             */
            int n = l * w;
            // compute adjacency matrix
            std::vector<std::vector<bool>> adj(n, std::vector<bool>(n));
            for (int i = 0; i < l; i++) {
                for (int j = 0; j < w; j++) {
                    int id = i + j * l;
                    // connect to the left
                    if (i > 0)
                        adj[id][id - 1] = adj[id - 1][id] = true;
                    // connect up
                    if (j > 0)
                        adj[id][id - l] = adj[id - l][id] = true;
                }
            }

            write_to_stream(adj,
                            "Rectangular_" + std::to_string(l) + "_x_" +
                                std::to_string(w),
                            std::cout);
        }
    } else if (circular >= 3) {
        int n = circular;
        // compute adjacency matrix
        std::vector<std::vector<bool>> adj(n, std::vector<bool>(n));
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            adj[i][j] = adj[j][i] = true;
        }

        write_to_stream(adj, "Circular_" + std::to_string(n), std::cout);
    } else if (linear >= 2) {
        int n = linear;
        // compute adjacency matrix
        std::vector<std::vector<bool>> adj(n, std::vector<bool>(n));
        for (int i = 1; i < n; i++) {
            adj[i][i - 1] = adj[i - 1][i] = true;
        }

        write_to_stream(adj, "Linear_" + std::to_string(n), std::cout);
    }
}
