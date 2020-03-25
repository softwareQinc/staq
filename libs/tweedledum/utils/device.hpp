/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Mathias Soeken
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "bit_matrix_rm.hpp"

#include <chrono>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace tweedledum {
/*! \brief Data-structure for the architecture of a quantum device.
 *
 * This data structure encapsulates the most essential properties of a physical
 * quantum device used by our mapping algorithms. These are the number of qubits
 * and an undirected coupling graph describing which pairs of qubits can
 * interact with each other.
 */
struct device {
    using edge_type = std::pair<uint32_t, uint32_t>;

    /*! \brief Pairs of qubit connections in the coupling graph. */
    std::vector<edge_type> edges;

    /*! \brief Number of qubits. */
    uint32_t num_vertices;

    device(uint32_t num_qubits = 0) : num_vertices(num_qubits) {}

    /*! \brief Create a device for a path topology.
     *
     * \param num_qubits Number of qubits
     */
    static device path(uint32_t num_qubits) {
        device arch(num_qubits);
        for (uint32_t i = 1; i < num_qubits; ++i) {
            arch.add_edge(i - 1, i);
        }
        return arch;
    }

    /*! \brief Create a device for a ring topology.
     *
     * \param num_qubits Number of qubits
     */
    static device ring(uint32_t num_qubits) {
        device arch(num_qubits);
        for (uint32_t i = 0; i < num_qubits; ++i) {
            arch.add_edge(i, (i + 1) % num_qubits);
        }
        return arch;
    }

    /*! \brief Create a device for a star topology.
     *
     * \param num_qubits Number of qubits
     */
    static device star(uint32_t num_qubits) {
        device arch(num_qubits);
        for (uint32_t i = 1; i < num_qubits; ++i) {
            arch.add_edge(0, i);
        }
        return arch;
    }

    /*! \brief Create a device for a grid topology.
     *
     * The device has `width * height` number of qubits.
     *
     * \param width Width of the grid
     * \param height Height of the grid
     */
    static device grid(uint32_t width, uint32_t height) {
        device arch(width * height);
        for (uint32_t x = 0; x < width; ++x) {
            for (uint32_t y = 0; y < height; ++y) {
                uint32_t e = y * width + x;
                if (x < width - 1) {
                    arch.add_edge(e, e + 1);
                }
                if (y < height - 1) {
                    arch.add_edge(e, e + width);
                }
            }
        }
        return arch;
    }

    /*! \brief Creates a device with a random topology.
     *
     * \param num_qubits Number of qubits
     * \param num_edges Number of edges in coupling graph
     */
    static device random(uint32_t num_qubits, uint32_t num_edges) {
        std::default_random_engine gen(
            std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<uint32_t> d1(0, num_qubits - 1);
        std::uniform_int_distribution<uint32_t> d2(1, num_qubits - 2);

        device arch(num_qubits);
        while (arch.edges.size() != num_edges) {
            uint32_t p = d1(gen);
            uint32_t q = (p + d2(gen)) % num_qubits;
            arch.add_edge(p, q);
        }
        return arch;
    }

    /*! \brief Add an edge between two vertices
     *
     * \param v Vertice identifier
     * \param w Vertice identifier
     */
    void add_edge(uint32_t v, uint32_t w) {
        assert(v <= num_vertices && w <= num_vertices);
        edge_type edge = {std::min(v, w), std::max(w, v)};
        if (std::find(edges.begin(), edges.end(), edge) == edges.end()) {
            edges.emplace_back(edge);
        }
    }

    /*! \brief Returns adjacency matrix of coupling graph. */
    bit_matrix_rm<> get_coupling_matrix() const {
        bit_matrix_rm<> matrix(num_vertices, num_vertices);

        for (auto const& [v, w] : edges) {
            matrix.at(v, w) = true;
            matrix.at(w, v) = true;
        }
        return matrix;
    }

    /*! \brief Returns distance matrix of coupling graph. */
    std::vector<std::vector<uint32_t>> get_distance_matrix() const {
        std::vector<std::vector<uint32_t>> matrix(
            num_vertices,
            std::vector<uint32_t>(num_vertices, num_vertices + 1));
        for (auto const& [v, w] : edges) {
            matrix[v][w] = 1;
            matrix[w][v] = 1;
        }
        for (auto v = 0u; v < num_vertices; ++v) {
            matrix[v][v] = 0;
        }
        for (auto k = 0u; k < num_vertices; ++k) {
            for (auto i = 0u; i < num_vertices; ++i) {
                for (auto j = 0u; j < num_vertices; ++j) {
                    if (matrix[i][j] > matrix[i][k] + matrix[k][j]) {
                        matrix[i][j] = matrix[i][k] + matrix[k][j];
                    }
                }
            }
        }
        return matrix;
    }
};

} // namespace tweedledum
