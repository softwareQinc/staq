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

#ifndef GRID_SYNTH_MATRIX_HPP_
#define GRID_SYNTH_MATRIX_HPP_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "rings.hpp"

namespace staq {
namespace grid_synth {

/*
 * Unitary matrices over the ring D[\omega] represented as elements of Z[\omega]
 * with smallest denominating exponent of base SQRT2 = k. Lemma 4 of
 * arXiv:1206.5236v4 implies that z_ and w_ have the same
 * denominating exponent, since |z_|^ + |w_|^2 = 1.
 */
class DOmegaMatrix {

  private:
    ZOmega u_;
    ZOmega t_;
    int_t k_; // power of SQRT2 in denominator
    unsigned int l_;

  public:
    DOmegaMatrix(const ZOmega& u, const ZOmega& t, const int_t& k,
                 const unsigned int& l)
        : u_(u), t_(t), k_(k), l_(l) {
        this->reduce();
        assert((l < 8) && (l >= 0));
    }

    ZOmega u() const { return u_; }
    ZOmega t() const { return t_; }

    int_t k() const { return k_; }
    unsigned int l() const { return l_; }

    int_t sde_u_sq() const {
        if (u_ == ZOmega(0))
            return 0;

        int_t s = 2 * k_;
        ZOmega u_sq = u_ * u_.conj();

        while (u_sq.is_reducible()) {
            u_sq = u_sq.reduce();
            s -= 1;
        }

        return s;
    }

    void reduce() {
        if (u_ == ZOmega(0) && t_ == ZOmega(0))
            return;
        while (u_.is_reducible() && t_.is_reducible()) {
            u_ = u_.reduce();
            t_ = t_.reduce();
            k_ -= 1;
        }
    }

    DOmegaMatrix dagger() const {
        return DOmegaMatrix(u_.conj(), -t_ * w_pow(-l_), k_, (8 - l_) % 8);
    }

    DOmegaMatrix mul_by_w(const int n) const {
        assert(-1 < n && n < 8);
        return (*this) * DOmegaMatrix(w_pow(n), ZOmega(0), 0, (2 * n) % 8);
    }

    DOmegaMatrix operator*(const DOmegaMatrix& B) const {
        return DOmegaMatrix(u_ * B.u() - t_.conj() * B.t() * w_pow(l_),
                            t_ * B.u() + u_.conj() * B.t() * w_pow(l_),
                            k_ + B.k(), (l_ + B.l()) % 8);
    }

    bool operator==(const DOmegaMatrix& B) const {
        return ((u_ == B.u()) && (t_ == B.t()) && (k_ == B.k()) &&
                (l_ == B.l()));
    }

    bool operator!=(const DOmegaMatrix& B) const { return !((*this) == B); }

}; // DOmegaMatrix

// Custom hash for DOmegaMatrix
struct DOmegaMatrixHash {
    std::size_t operator()(const DOmegaMatrix& D) const {
        using str_t = str_t;
        ZOmega u = D.u();
        ZOmega t = D.t();

        str_t u_str = u.a().get_str() + u.b().get_str() + u.c().get_str() +
                      u.d().get_str() + std::to_string(D.l()) + D.k().get_str();
        str_t t_str = t.a().get_str() + t.b().get_str() + t.c().get_str() +
                      t.d().get_str();

        std::size_t h1 = std::hash<str_t>{}(u_str);
        std::size_t h2 = std::hash<str_t>{}(t_str);

        return h1 ^ (h2 << 1);
    }
};

using domega_matrix_table_t =
    std::unordered_map<DOmegaMatrix, str_t, DOmegaMatrixHash>;

inline std::ostream& operator<<(std::ostream& os, const DOmegaMatrix& M) {
    os << "----" << std::endl
       << "u = " << M.u() << std::endl
       << "t = " << M.t() << std::endl
       << "k = " << M.k() << std::endl
       << "l = " << M.l() << std::endl
       << "----" << std::endl;
    return os;
}

// some important matrices
const DOmegaMatrix I(ZOmega(1), ZOmega(0), 0, 0);
const DOmegaMatrix H(ZOmega(1), ZOmega(1), 1, 4);
const DOmegaMatrix T(ZOmega(1), ZOmega(0), 0, 1);
const DOmegaMatrix S(ZOmega(1), ZOmega(0), 0, 2);

inline DOmegaMatrix domega_matrix_from_str(str_t str) {
    // reverse(str.begin(),str.end());
    DOmegaMatrix prod = I;
    for (auto& ele : str) {
        if (ele == 'H')
            prod = prod * H;
        else if (ele == 'T')
            prod = prod * T;
        else if (ele == 'S')
            prod = prod * S;
        else if (ele == 'W')
            prod = prod.mul_by_w(1);
        else if (ele == 'I')
            continue;
        else {
            std::cout << "In domega_matrix_from_str, unrecognized character "
                      << ele << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return prod;
}

// Accept a string of operator labels and reduce products to consist purely of
// H, T, and S.
inline str_t simplify_str(str_t str) {
    str.erase(std::remove(str.begin(), str.end(), 'I'), str.end());
    std::size_t len = str.size();
    if (str.empty())
        return "I";
    if (str.size() == 1)
        return str;

    str_t new_str = "";
    std::size_t first = 0;
    std::size_t second = 1;

    while (first < len) {
        if (second >= len) {
            if (str[first] == 'I')
                break;
            new_str += str[first];
            break;
        }
        if (str[first] == 'I') {
            first++;
            second++;
            continue;
        } else if (str[first] == 'H') {
            if (str[second] == 'H') {
                first += 2;
                second += 2;
                continue;
            }
            new_str += "H";
            first++;
            second++;
        } else if (str[first] == 'T') {
            if (str[second] == 'T') {
                new_str += "S";
                first += 2;
                second += 2;
                continue;
            }
            new_str += "T";
            first++;
            second++;
        } else if (str[first] == 'S') {
            new_str += "S";
            first++;
            second++;
        } else if (str[first] == 'W') {
            new_str += "W";
            first++;
            second++;
            continue;
        } else {
            std::cout << "In simplify_str, unrecognized character in string "
                      << str[first] << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return new_str;
}

/*
 *  Reduce a string containing H, T, and S as far as is possible
 */
inline str_t full_simplify_str(str_t str) {
    if (str.size() == 1)
        return str;
    size_t last_len = str.size();
    size_t curr_len = 0;
    str_t curr_str = str;
    while (curr_len < last_len) {
        last_len = curr_str.size();
        curr_str = simplify_str(curr_str);
        curr_len = curr_str.size();
    }

    return curr_str;
}

// ========================================================================= //
// Note: These functions are no longer used by grid_synth.
// ========================================================================= //

// Generate the set of all unitary matrices with SDE less than three
inline domega_matrix_table_t generate_s3_table() {
    using namespace std;
    using arr_t = array<str_t, 8>;

    domega_matrix_table_t s3_table;
    arr_t base = {"I", "T", "TT", "TTT", "TTTT", "TTTTT", "TTTTTT", "TTTTTTT"};
    arr_t wstr = {"", "W", "WW", "WWW", "WWWW", "WWWWW", "WWWWWW", "WWWWWWW"};

    for (int i = 0; i < 8; i++) {
        DOmegaMatrix D = domega_matrix_from_str(base[i]);
        str_t str = base[i];
        for (int n = 0; n < 8; n++) {
            // if((D.mul_by_w(n)).sde_u_sq()>3) continue;
            s3_table.insert({D.mul_by_w(n), str + wstr[n]});
        }

        for (int j = 0; j < 8; j++) {
            D = domega_matrix_from_str(base[i] + "H" + base[j]);
            str = simplify_str(base[i] + "H" + base[j]);
            for (int n = 0; n < 8; n++) {
                // if((D.mul_by_w(n)).sde_u_sq()>3) continue;
                s3_table.insert({D.mul_by_w(n), str + wstr[n]});
            }

            for (int k = 0; k < 8; k++) {
                D = domega_matrix_from_str(base[i] + "H" + base[j] + "H" +
                                           base[k]);
                str = simplify_str(base[i] + "H" + base[j] + "H" + base[k]);
                for (int n = 0; n < 8; n++) {
                    // if((D.mul_by_w(n)).sde_u_sq()>3) continue;
                    s3_table.insert({D.mul_by_w(n), str + wstr[n]});
                }

                for (int l = 0; l < 8; l++) {
                    D = domega_matrix_from_str(base[i] + "H" + base[j] + "H" +
                                               base[k] + "H" + base[l]);
                    str = simplify_str(base[i] + "H" + base[j] + "H" + base[k] +
                                       "H" + base[l]);
                    for (int n = 0; n < 8; n++) {
                        // if((D.mul_by_w(n)).sde_u_sq()>3) continue;
                        s3_table.insert({D.mul_by_w(n), str + wstr[n]});
                    }
                }
            }
        }
    }
    return s3_table;
}

inline void write_s3_table(const str_t& filename,
                           const domega_matrix_table_t& s3_table) {
    using namespace std;
    ofstream file_stream;
    file_stream.open(filename);
    for (auto& it : s3_table) {
        DOmegaMatrix mat = it.first;
        str_t op_str = it.second;
        file_stream << mat.u().csv_str() << "," << mat.t().csv_str() << ","
                    << mat.k() << "," << mat.l() << "," << op_str << '\n';
    }
    file_stream.close();
}

inline domega_matrix_table_t read_s3_table(const str_t& filename) {
    using namespace std;
    domega_matrix_table_t s3_table;
    ifstream file_stream(filename);
    str_t curr_line;
    while (getline(file_stream, curr_line)) {
        stringstream line_stream(curr_line);
        curr_line.erase(remove(curr_line.begin(), curr_line.end(), 'I'),
                        curr_line.end());
        vector<str_t> line;
        str_t entry;
        while (getline(line_stream, entry, ',')) {
            line.push_back(entry);
        }
        DOmegaMatrix mat(
            ZOmega(stoi(line[0]), stoi(line[1]), stoi(line[2]), stoi(line[3])),
            ZOmega(stoi(line[4]), stoi(line[5]), stoi(line[6]), stoi(line[7])),
            stoi(line[8]), stoi(line[9]));
        s3_table[mat] = line[10];
    }
    return s3_table;
}

} // namespace grid_synth
} // namespace staq

#endif // GRID_SYNTH_MATRIX_HPP_
