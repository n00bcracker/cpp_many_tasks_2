#pragma once

#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <vector>

using std::vector;

class Matrix {
    vector<vector<uint64_t>> matrix_;
    size_t rows_num_, cols_num_;

public:
    Matrix(size_t rows_num, size_t cols_num)
        : matrix_(rows_num, vector<uint64_t>(cols_num, 0)),
          rows_num_(rows_num),
          cols_num_(cols_num) {
    }

    explicit Matrix(size_t rows_num) : Matrix(rows_num, rows_num) {
    }

    size_t Rows() const {
        return rows_num_;
    }

    size_t Columns() const {
        return cols_num_;
    }

    uint64_t& operator()(size_t row_idx, size_t col_idx) {
        return matrix_[row_idx][col_idx];
    }

    uint64_t operator()(size_t row_idx, size_t col_idx) const {
        return matrix_[row_idx][col_idx];
    }

    Matrix& operator+=(const Matrix& rhs) {
        for (size_t i = 0; i < rows_num_; ++i) {
            for (size_t j = 0; j < cols_num_; ++j) {
                matrix_[i][j] += rhs.matrix_[i][j];
            }
        }

        return *this;
    }

    Matrix operator-() const {
        Matrix negative(rows_num_, cols_num_);
        for (size_t i = 0; i < negative.rows_num_; ++i) {
            for (size_t j = 0; j < negative.cols_num_; ++j) {
                negative.matrix_[i][j] = -matrix_[i][j];
            }
        }

        return negative;
    }

    Matrix& operator-=(const Matrix& rhs) {
        return (*this) += -rhs;
    }

    friend Matrix operator*(const Matrix& lhs, const Matrix& rhs);

    Matrix& operator*=(const Matrix& rhs) {
        return *this = *this * rhs;
    }

    Matrix Pow(size_t pow);
};

Matrix operator+(Matrix lhs, const Matrix& rhs) {
    return lhs += rhs;
}

Matrix operator-(Matrix lhs, const Matrix& rhs) {
    return lhs -= rhs;
}

Matrix operator*(const Matrix& lhs, const Matrix& rhs) {
    Matrix dot(lhs.rows_num_, rhs.cols_num_);
    for (size_t i = 0; i < dot.rows_num_; ++i) {
        for (size_t j = 0; j < dot.cols_num_; ++j) {
            uint64_t sum = 0;
            for (size_t z = 0; z < lhs.cols_num_; ++z) {
                sum += lhs.matrix_[i][z] * rhs.matrix_[z][j];
            }
            dot.matrix_[i][j] = sum;
        }
    }

    return dot;
}

Matrix Transpose(const Matrix& m) {
    Matrix result(m.Columns(), m.Rows());
    for (size_t i = 0; i < result.Rows(); ++i) {
        for (size_t j = 0; j < result.Columns(); ++j) {
            result(i, j) = m(j, i);
        }
    }

    return result;
}

Matrix Identity(size_t rows_num) {
    Matrix result(rows_num);
    for (size_t i = 0; i < result.Rows(); ++i) {
        result(i, i) = 1;
    }

    return result;
}

Matrix Matrix::Pow(size_t pow) {
    Matrix res = Identity(rows_num_);
    Matrix m = *this;
    do {
        if (pow & 1) {
            res *= m;
        }
        m *= m;
        pow = pow >> 1;
    } while (pow);

    return res;
}