#pragma once

#include <sys/types.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <vector>

template <class T>
class Matrix;

template <typename L, typename T>
struct Glue {
    const L& left;
    const Matrix<T>& right;
};

struct OptMult {
    size_t left_part_end;
    size_t opt_op_num;
};

template <typename L, typename T>
void DFS(const Glue<L, T>& node, std::vector<const Matrix<T>*>& res) {
    if constexpr (std::is_same_v<decltype(node.left), decltype(node.right)>) {
        res.emplace_back(&node.left);
    } else {
        DFS(node.left, res);
    }

    res.emplace_back(&node.right);
}

template <class T>
class Matrix {
public:
    Matrix() : rows_num_(0), cols_num_(0) {
    }

    Matrix(size_t rows_num, size_t cols_num)
        : matrix_(rows_num, std::vector<T>(cols_num, T())),
          rows_num_(rows_num),
          cols_num_(cols_num) {
    }

    explicit Matrix(size_t rows_num) : Matrix(rows_num, rows_num) {
    }

    Matrix(std::vector<std::vector<T>> matrix)
        : matrix_(std::move(matrix)), rows_num_(matrix_.size()), cols_num_(matrix_[0].size()) {
    }

    Matrix(std::initializer_list<std::vector<T>> rows) : Matrix() {
        for (auto& row : rows) {
            matrix_.emplace_back(std::move(row));
        }

        rows_num_ = matrix_.size();
        cols_num_ = !matrix_.empty() ? matrix_[0].size() : 0;
    }

    size_t Rows() const {
        return rows_num_;
    }

    size_t Columns() const {
        return cols_num_;
    }

    T& operator()(size_t row_idx, size_t col_idx) {
        return matrix_[row_idx][col_idx];
    }

    const T& operator()(size_t row_idx, size_t col_idx) const {
        return matrix_[row_idx][col_idx];
    }

    friend Matrix operator+(Matrix lhs, const Matrix& rhs) {
        if ((lhs.rows_num_ != rhs.rows_num_) || (lhs.cols_num_ != rhs.cols_num_)) {
            throw std::runtime_error("Wrong sizes");
        }

        for (size_t i : std::views::iota(0u, lhs.rows_num_)) {
            for (size_t j : std::views::iota(0u, lhs.cols_num_)) {
                lhs(i, j) = lhs(i, j) + rhs(i, j);
            }
        }

        return lhs;
    }

    Matrix operator-() const {
        Matrix negative(rows_num_, cols_num_);
        for (size_t i : std::views::iota(0u, rows_num_)) {
            for (size_t j : std::views::iota(0u, cols_num_)) {
                negative(i, j) = -(*this)(i, j);
            }
        }

        return negative;
    }

    friend Matrix operator-(Matrix lhs, const Matrix& rhs) {
        if ((lhs.rows_num_ != rhs.rows_num_) || (lhs.cols_num_ != rhs.cols_num_)) {
            throw std::runtime_error("Wrong sizes");
        }

        for (size_t i : std::views::iota(0u, lhs.rows_num_)) {
            for (size_t j : std::views::iota(0u, lhs.cols_num_)) {
                lhs(i, j) = lhs(i, j) - rhs(i, j);
            }
        }

        return lhs;
    }

private:
    void ComputeOptMultNum(size_t begin, size_t end, const std::vector<const Matrix*>& matrices,
                           auto& op_nums) {
        std::optional<OptMult> opt_ops;
        for (size_t i = begin; i < end; ++i) {
            if (i != begin && op_nums[begin][i].opt_op_num == 0) {
                ComputeOptMultNum(begin, i, matrices, op_nums);
            }
            size_t left_part_op_num = op_nums[begin][i].opt_op_num;

            if (i + 1 != end && op_nums[i + 1][end].opt_op_num == 0) {
                ComputeOptMultNum(i + 1, end, matrices, op_nums);
            }
            size_t right_part_op_num = op_nums[i + 1][end].opt_op_num;

            size_t overall_ops =
                matrices[begin]->Rows() * matrices[i]->Columns() * matrices[end]->Columns();
            overall_ops += left_part_op_num + right_part_op_num;
            if (!opt_ops.has_value() || overall_ops < opt_ops->opt_op_num) {
                opt_ops = {i, overall_ops};
            }
        }

        op_nums[begin][end] = *opt_ops;
    }

    Matrix Dot(const Matrix& rhs) {
        Matrix dot(rows_num_, rhs.cols_num_);
        for (size_t i = 0; i < dot.rows_num_; ++i) {
            for (size_t j = 0; j < dot.cols_num_; ++j) {
                T sum(0);
                for (size_t z = 0; z < cols_num_; ++z) {
                    sum += matrix_[i][z] * rhs.matrix_[z][j];
                }
                dot.matrix_[i][j] = sum;
            }
        }

        return dot;
    }

    Matrix MultiplicateMatrices(size_t begin, size_t end,
                                const std::vector<const Matrix*>& matrices, const auto& op_nums) {
        auto left_part_end = op_nums[begin][end].left_part_end;
        Matrix left = left_part_end != begin
                          ? MultiplicateMatrices(begin, left_part_end, matrices, op_nums)
                          : *matrices[begin];
        Matrix right = left_part_end + 1 != end
                           ? MultiplicateMatrices(left_part_end + 1, end, matrices, op_nums)
                           : *matrices[end];
        return left.Dot(right);
    }

public:
    template <typename L>
    Matrix(const Glue<L, T>& tree) {
        std::vector<const Matrix*> operands;
        DFS(tree, operands);

        for (size_t i : std::views::iota(1u, operands.size())) {
            if (operands[i - 1]->Columns() != operands[i]->Rows()) {
                throw std::runtime_error("Wrong sizes");
            }
        }

        std::vector<std::vector<OptMult>> op_nums(operands.size(),
                                                  std::vector<OptMult>(operands.size(), {0, 0}));
        ComputeOptMultNum(0, operands.size() - 1, operands, op_nums);
        *this = MultiplicateMatrices(0, operands.size() - 1, operands, op_nums);
    }

private:
    std::vector<std::vector<T>> matrix_;
    size_t rows_num_, cols_num_;
};

template <typename L>
concept SuitableMulOperand = requires(L left) { Glue(left); } || requires(L left) { Matrix(left); };

template <SuitableMulOperand L, typename T>
Glue<L, T> operator*(const L& left, const Matrix<T>& right) {
    return Glue<L, T>{left, right};
}