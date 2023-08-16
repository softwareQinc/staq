#ifndef TYPES_HPP
#define TYPES_HPP

#include <Eigen/Dense>
#include <array>
#include <cassert>
#include <complex>
#include <gmpxx.h>
#include <queue>
#include <string>

#include "complex.hpp"

namespace staq {
namespace grid_synth {
using int_t = mpz_class;
using real_t = mpf_class;
using cplx_t = complex<real_t>;
using str_t = std::string;

namespace NoEigen {
template <typename T = real_t>
struct row_vec_t;

// 2x1 column vector
template <typename T = real_t>
struct col_vec_t {
    std::array<T, 2> col;

    T& operator()(int index) {
        assert(index >= 0 && index < 2);
        return col[index];
    }

    T operator()(int index) const {
        assert(index >= 0 && index < 2);
        return col[index];
    }

    T& operator[](int index) { return this->operator()(index); }

    T operator[](int index) const { return this->operator()(index); }

    col_vec_t operator-(const col_vec_t& rhs) const {
        return {col[0] - rhs.col[0], col[1] - rhs.col[1]};
    }

    col_vec_t operator+(const col_vec_t& rhs) const {
        return {col[0] + rhs.col[0], col[1] + rhs.col[1]};
    }

    row_vec_t<T> transpose() const { return row_vec_t<T>{col}; }

    friend col_vec_t operator*(T lhs, const col_vec_t& rhs) {
        return {lhs * rhs.col[0], lhs * rhs.col[1]};
    }

    friend col_vec_t operator*(const col_vec_t& lhs, T rhs) {
        return {lhs.col[0] * rhs, lhs.col[1] * rhs};
    }

    friend std::ostream& operator<<(std::ostream& os, const col_vec_t& rhs) {
        os << rhs.col[0] << '\n' << rhs.col[1];
        return os;
    }
};

// 1x2 row vector
template <typename T>
struct row_vec_t {
    std::array<T, 2> row;

    T& operator()(int index) {
        assert(index >= 0 && index < 2);
        return row[index];
    }

    T operator()(int index) const {
        assert(index >= 0 && index < 2);
        return row[index];
    }

    T& operator[](int index) { return this->operator()(index); }

    T operator[](int index) const { return this->operator()(index); }

    row_vec_t operator-(const row_vec_t& rhs) const {
        return {row[0] - rhs.row[0], row[1] - rhs.row[1]};
    }

    row_vec_t operator+(const row_vec_t& rhs) const {
        return {row[0] + rhs.row[0], row[1] + rhs.row[1]};
    }

    col_vec_t<T> transpose() const { return col_vec_t<T>{row}; }

    friend row_vec_t operator*(T lhs, const row_vec_t& rhs) {
        return {lhs * rhs.row[0], lhs * rhs.row[1]};
    }

    friend row_vec_t operator*(const row_vec_t& lhs, T rhs) {
        return {lhs.row[0] * rhs, lhs.row[1] * rhs};
    }

    friend std::ostream& operator<<(std::ostream& os, const row_vec_t& rhs) {
        os << rhs.row[0] << ' ' << rhs.row[1];
        return os;
    }
};

// 2x2 matrix
template <typename T = real_t>
struct mat_t {
    std::array<row_vec_t<T>, 2> data_;

    row_vec_t<T>& operator()(int index) {
        assert(index >= 0 && index < 2);
        return data_[index];
    }

    row_vec_t<T> operator()(int index) const {
        assert(index >= 0 && index < 2);
        return data_[index];
    }

    row_vec_t<T>& operator[](int index) { return this->operator()(index); }

    row_vec_t<T> operator[](int index) const { return this->operator()(index); }

    mat_t transpose() const {
        return {data_[0][0], data_[1][0], data_[0][1], data_[1][1]};
    }

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

        return std::max(expr1, expr2);
    }

    mat_t inverse() const {
        assert(determinant() != 0);
        return (1. / determinant()) *
               mat_t{data_[1][1], -data_[0][1], -data_[1][0], data_[0][0]};
    }

    T operator()(int i, int j) const {
        assert(i >= 0 && i < 2);
        return this->operator()(i).operator()(j);
    }

    mat_t operator-(const mat_t& rhs) const {
        return {data_[0] - rhs.data_[0], data_[1] - rhs.data_[1]};
    }

    mat_t operator+(const mat_t& rhs) const {
        return {data_[0] + rhs.data_[0], data_[1] + rhs.data_[1]};
    }

    friend mat_t operator*(T lhs, const mat_t& rhs) {
        return {lhs * rhs[0], lhs * rhs[1]};
    }

    friend mat_t operator*(const mat_t& lhs, T rhs) {
        return {lhs[0] * rhs, lhs[1] * rhs};
    }

    friend mat_t operator*(const mat_t& lhs, const mat_t& rhs) {
        return {lhs[0][0] * rhs[0][0] + lhs[0][1] * rhs[1][0],
                lhs[0][0] * rhs[0][1] + lhs[0][1] * rhs[1][1],
                lhs[1][0] * rhs[0][0] + lhs[1][1] * rhs[1][0],
                lhs[1][0] * rhs[0][1] + lhs[1][1] * rhs[1][1]};
    }

    friend row_vec_t<T> operator*(const row_vec_t<T>& lhs, const mat_t& rhs) {
        return {lhs[0] * rhs[0][0] + lhs[1] * rhs[1][0],
                lhs[0] * rhs[0][1] + lhs[1] * rhs[1][1]};
    }

    friend col_vec_t<T> operator*(const mat_t& lhs, const col_vec_t<T>& rhs) {
        return {lhs[0][0] * rhs[0] + lhs[0][1] * rhs[1],
                lhs[1][0] * rhs[0] + lhs[1][1] * rhs[1]};
    }

    friend std::ostream& operator<<(std::ostream& os, const mat_t& rhs) {
        os << rhs[0][0] << ' ' << rhs[0][1] << '\n';
        os << rhs[1][0] << ' ' << rhs[1][1];
        return os;
    }
};

template <class T>
T operator*(const row_vec_t<T>& lhs, const col_vec_t<T>& rhs) {
    return lhs[0] * rhs[0] + lhs[1] * rhs[1];
}

template <class T>
mat_t<T> operator*(const col_vec_t<T>& lhs, const row_vec_t<T>& rhs) {
    return {lhs[0] * rhs[0], lhs[0] * rhs[1], lhs[1] * rhs[0], lhs[1] * rhs[1]};
}

// template deduction rules
template <class T>
row_vec_t(T, T) -> row_vec_t<real_t>;

template <class T>
col_vec_t(T, T) -> col_vec_t<real_t>;

template <class T>
mat_t(row_vec_t<T>, row_vec_t<T>) -> mat_t<real_t>;

} // namespace NoEigen

// using vec_t = Eigen::Matrix<real_t, 2, 1>;
// using mat_t = Eigen::Matrix<real_t, 2, 2>;

using vec_t = NoEigen::col_vec_t<real_t>;
using mat_t = NoEigen::mat_t<real_t>;

using int_vec_t = std::vector<int_t>;
using int_queue_t = std::queue<int_t>;
} // namespace grid_synth
} // namespace staq

#endif // TYPES_HPP
