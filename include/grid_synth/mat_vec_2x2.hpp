#ifndef GRID_SYNTH_MAT_VEC_2X2_HPP_
#define GRID_SYNTH_MAT_VEC_2X2_HPP_

#include <array>
#include <cassert>
#include <gmpxx.h>

namespace staq {
namespace grid_synth {

using real_t = mpf_class;

template <typename T = real_t>
struct row_vec2_t;

// 2x1 column vector
template <typename T = real_t>
class col_vec2_t {
    std::array<T, 2> col_;

  public:
    col_vec2_t() = default;

    col_vec2_t(T x_0, T x_1) : col_{x_0, x_1} {}

    row_vec2_t<T> transpose() const { return {col_[0], col_[1]}; }

    T& operator()(int index) {
        assert(index >= 0 && index < 2);
        return col_[index];
    }

    T operator()(int index) const {
        assert(index >= 0 && index < 2);
        return col_[index];
    }

    T& operator[](int index) { return this->operator()(index); }

    T operator[](int index) const { return this->operator()(index); }

    col_vec2_t operator-(const col_vec2_t& rhs) const {
        return {col_[0] - rhs.col_[0], col_[1] - rhs.col_[1]};
    }

    col_vec2_t operator+(const col_vec2_t& rhs) const {
        return {col_[0] + rhs.col_[0], col_[1] + rhs.col_[1]};
    }

    bool operator==(const col_vec2_t& other) const { return col_ == other.row; }

    bool operator!=(const col_vec2_t& other) const { return !(*this == other); }

    friend col_vec2_t operator*(T lhs, const col_vec2_t& rhs) {
        return {lhs * rhs.col_[0], lhs * rhs.col_[1]};
    }

    friend col_vec2_t operator*(const col_vec2_t& lhs, T rhs) {
        return {lhs.col_[0] * rhs, lhs.col_[1] * rhs};
    }

    friend std::ostream& operator<<(std::ostream& os, const col_vec2_t& rhs) {
        os << rhs.col_[0] << '\n' << rhs.col_[1];
        return os;
    }
};

// 1x2 row vector
template <typename T>
class row_vec2_t {
    std::array<T, 2> row_{};

  public:
    row_vec2_t() = default;

    row_vec2_t(T x_0, T x_1) : row_{x_0, x_1} {}

    col_vec2_t<T> transpose() const { return {row_[0], row_[1]}; }

    T& operator()(int index) {
        assert(index >= 0 && index < 2);
        return row_[index];
    }

    T operator()(int index) const {
        assert(index >= 0 && index < 2);
        return row_[index];
    }

    T& operator[](int index) { return this->operator()(index); }

    T operator[](int index) const { return this->operator()(index); }

    row_vec2_t operator-(const row_vec2_t& rhs) const {
        return {row_[0] - rhs.row_[0], row_[1] - rhs.row_[1]};
    }

    row_vec2_t operator+(const row_vec2_t& rhs) const {
        return {row_[0] + rhs.row_[0], row_[1] + rhs.row_[1]};
    }

    bool operator==(const row_vec2_t& other) const {
        return row_ == other.row_;
    }

    bool operator!=(const row_vec2_t& other) const { return !(*this == other); }

    friend row_vec2_t operator*(T lhs, const row_vec2_t& rhs) {
        return {lhs * rhs.row_[0], lhs * rhs.row_[1]};
    }

    friend row_vec2_t operator*(const row_vec2_t& lhs, T rhs) {
        return {lhs.row_[0] * rhs, lhs.row_[1] * rhs};
    }

    friend std::ostream& operator<<(std::ostream& os, const row_vec2_t& rhs) {
        os << rhs.row_[0] << ' ' << rhs.row_[1];
        return os;
    }
};

// 2x2 matrix
template <typename T = real_t>
class mat2_t {
    std::array<row_vec2_t<T>, 2> data_{};

  public:
    mat2_t() = default;

    mat2_t(T m_00, T m_01, T m_10, T m_11)
        : data_{row_vec2_t{m_00, m_01}, row_vec2_t{m_10, m_11}} {}

    T determinant() const {
        return data_[0][0] * data_[1][1] - data_[0][1] * data_[1][0];
    }

    T trace() const { return data_[0][0] + data_[1][1]; }

    T norm() const {
        T a = data_[0][0];
        T b = data_[0][1];
        T c = data_[1][0];
        T d = data_[1][1];

        T expr1 = sqrt(a * a + b * b + c * c + d * d -
                       sqrt(((b + c) * (b + c) + (a - d) * (a - d)) *
                            ((b - c) * (b - c) + (a + d) * (a + d)))) /
                  sqrt(2);
        T expr2 = sqrt(a * a + b * b + c * c + d * d +
                       sqrt(((b + c) * (b + c) + (a - d) * (a - d)) *
                            ((b - c) * (b - c) + (a + d) * (a + d)))) /
                  sqrt(2);

        // TODO doesn't look like we need max here, expr1 > expr2 always; the max
        // comes from Mathematica: FullSimplify[Norm[{{a, b}, {c, d}}],
        //  Assumptions -> Element[a, Reals] && Element[b, Reals] &&
        //  Element[c, Reals] && Element[d, Reals]]
        return std::max(expr1, expr2);
    }

    mat2_t inverse() const {
        assert(determinant() != 0);
        return (1. / determinant()) *
               mat2_t{data_[1][1], -data_[0][1], -data_[1][0], data_[0][0]};
    }

    mat2_t transpose() const {
        return {data_[0][0], data_[1][0], data_[0][1], data_[1][1]};
    }

    row_vec2_t<T>& operator()(int index) {
        assert(index >= 0 && index < 2);
        return data_[index];
    }

    row_vec2_t<T> operator()(int index) const {
        assert(index >= 0 && index < 2);
        return data_[index];
    }

    row_vec2_t<T>& operator[](int index) { return this->operator()(index); }

    row_vec2_t<T> operator[](int index) const {
        return this->operator()(index);
    }

    T operator()(int i, int j) const {
        assert(i >= 0 && i < 2);
        return this->operator()(i).operator()(j);
    }

    mat2_t operator-(const mat2_t& rhs) const {
        return {
            data_[0][0] - rhs.data_[0][0], data_[0][1] - rhs.data_[0][1],
                data_[1][0] - rhs.data_[1][0], data_[1][1] - rhs.data_[1][1]
        };
    }

    mat2_t operator+(const mat2_t& rhs) const {
        return {
            data_[0][0] + rhs.data_[0][0], data_[0][1] + rhs.data_[0][1],
                data_[1][0] + rhs.data_[1][0], data_[1][1] + rhs.data_[1][1]
        };
    }

    bool operator==(const mat2_t& other) const { return data_ == other.data_; }

    bool operator!=(const mat2_t& other) const { return !(*this == other); }

    friend mat2_t operator*(T lhs, const mat2_t& rhs) {
        return {lhs * rhs[0][0], lhs * rhs[0][1], lhs * rhs[1][0],
                lhs * rhs[1][1]};
    }

    friend mat2_t operator*(const mat2_t& lhs, T rhs) {
        return {lhs[0][0] * rhs, lhs[0][1] * rhs, lhs[1][0] * rhs,
                lhs[1][1] * rhs};
    }

    friend mat2_t operator*(const mat2_t& lhs, const mat2_t& rhs) {
        return {lhs[0][0] * rhs[0][0] + lhs[0][1] * rhs[1][0],
                lhs[0][0] * rhs[0][1] + lhs[0][1] * rhs[1][1],
                lhs[1][0] * rhs[0][0] + lhs[1][1] * rhs[1][0],
                lhs[1][0] * rhs[0][1] + lhs[1][1] * rhs[1][1]};
    }

    friend row_vec2_t<T> operator*(const row_vec2_t<T>& lhs,
                                   const mat2_t& rhs) {
        return {lhs[0] * rhs[0][0] + lhs[1] * rhs[1][0],
                lhs[0] * rhs[0][1] + lhs[1] * rhs[1][1]};
    }

    friend col_vec2_t<T> operator*(const mat2_t& lhs,
                                   const col_vec2_t<T>& rhs) {
        return {lhs[0][0] * rhs[0] + lhs[0][1] * rhs[1],
                lhs[1][0] * rhs[0] + lhs[1][1] * rhs[1]};
    }

    friend std::ostream& operator<<(std::ostream& os, const mat2_t& rhs) {
        os << rhs[0][0] << ' ' << rhs[0][1] << '\n';
        os << rhs[1][0] << ' ' << rhs[1][1];
        return os;
    }
};

template <class T>
T operator*(const row_vec2_t<T>& lhs, const col_vec2_t<T>& rhs) {
    return lhs[0] * rhs[0] + lhs[1] * rhs[1];
}

template <class T>
mat2_t<T> operator*(const col_vec2_t<T>& lhs, const row_vec2_t<T>& rhs) {
    return {lhs[0] * rhs[0], lhs[0] * rhs[1], lhs[1] * rhs[0], lhs[1] * rhs[1]};
}

// template deduction rules
template <class T>
row_vec2_t(T, T) -> row_vec2_t<real_t>;

template <class T>
col_vec2_t(T, T) -> col_vec2_t<real_t>;

template <class T>
mat2_t(row_vec2_t<T>, row_vec2_t<T>) -> mat2_t<real_t>;

} /* namespace grid_synth */
} /* namespace staq */

#endif /* GRID_SYNTH_MAT_VEC_2X2_HPP_ */
