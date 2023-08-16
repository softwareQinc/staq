#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

#include "rings.hpp"

namespace staq{
namespace grid_synth {



/*
 * Matrices over the ring D[\omega] represented as elements of Z[\omega] with
 * smallest denominating exponent of base SQRT2 = k. Lemma 4 of
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
        assert((l < 8) and (l >= 0));
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
        while (u_.is_reducible() and t_.is_reducible()) {
            u_ = u_.reduce();
            t_ = t_.reduce();
            k_ -= 1;
        }
    }

    DOmegaMatrix dagger() const {
        return DOmegaMatrix(u_.conj(), -t_ * w_pow(-l_), k_, (8 - l_) % 8);
    }

    DOmegaMatrix mul_by_w(const int n) const {
        assert(-1 < n and n < 8);
        return (*this) * DOmegaMatrix(w_pow(n), ZOmega(0), 0, (8 - n) % 8);
    }

    DOmegaMatrix operator*(const DOmegaMatrix& B) const {
        return DOmegaMatrix(u_ * B.u() - t_.conj() * B.t() * w_pow(l_),
                            t_ * B.u() + u_.conj() * B.t() * w_pow(l_),
                            k_ + B.k(), (l_ + B.l()) % 8);
    }

    bool operator==(const DOmegaMatrix& B) const {
        return ((u_ == B.u()) and (t_ == B.t()) and (k_ == B.k()) and
                (l_ == B.l()));
    }

    bool operator!=(const DOmegaMatrix& B) const { return not((*this) == B); }

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

using domega_matrix_table =
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
        else if (ele == 'w')
            prod = prod.mul_by_w(1);
        else if (ele == 'I')
            continue;
        else {
            std::cout << "Failed to identify string character " << ele
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return prod;
}

// Accept a string of operator labels and reduce products to consist purely of
// H, T, S, and Z.
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
        } else if (str[first] == 'w') {
            new_str += "w"; 
            first++;
            second++;
            continue;
        } else {
            std::cout << "Unrecognized character in operator string."
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return new_str;
}

// Generate the set of all unitary matrices with SDE less than three
inline domega_matrix_table generate_s3_table() {
    using namespace std;
    using arr_t = array<str_t, 8>;

    domega_matrix_table s3_table;
    arr_t base = {"I", "T", "TT", "TTT", "TTTT", "TTTTT", "TTTTTT", "TTTTTTT"};
    arr_t wstr = {"", "w", "ww", "www", "wwww", "wwwww", "wwwwww", "wwwwwww"};

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

} // namespace grid_synth
} // namespace staq

#endif // MATRIX_HPP
