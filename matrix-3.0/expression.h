#pragma once

#include "tokenizer.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <map>

class Expression {
public:
    explicit Expression(const std::map<std::string, size_t>& vars)
        : vars_(vars), coeffs_(vars.size() + 1, 0) {
    }

    virtual ~Expression() = default;
    virtual std::vector<int64_t> Evaluate() = 0;

protected:
    const std::map<std::string, size_t>& vars_;
    std::vector<int64_t> coeffs_;
};

class Constant : public Expression {
public:
    Constant(Line& line, const std::map<std::string, size_t>& vars);
    std::vector<int64_t> Evaluate() override;

private:
    std::unique_ptr<Expression> constant_ = nullptr;
    std::unique_ptr<Expression> sum_ = nullptr;
};

class Mul : public Expression {
public:
    Mul(Line& line, const std::map<std::string, size_t>& vars);
    Mul(Mul&& other);
    std::vector<int64_t> Evaluate() override;

private:
    std::unique_ptr<Expression> mul_ = nullptr;
    std::unique_ptr<Expression> constant_ = nullptr;
};

class Sum : public Expression {
public:
    Sum(Line& line, const std::map<std::string, size_t>& vars);
    Sum(Sum&& other);
    std::vector<int64_t> Evaluate() override;

private:
    std::unique_ptr<Expression> sum_ = nullptr;
    std::unique_ptr<Expression> mul_ = nullptr;
    char op_;
};

Constant::Constant(Line& line, const std::map<std::string, size_t>& vars) : Expression(vars) {
    Token token = line.Get();
    if (token.type == TokenType::kNumber) {
        coeffs_.back() = token.number;
        line.Next();
    } else if (token.type == TokenType::kVar) {
        coeffs_[vars.at(token.var_name)] = 1;
        line.Next();
    } else if (token.type == TokenType::kSymbol) {
        if (token.symbol == '-') {
            line.Next();
            constant_ = std::unique_ptr<Expression>(new Constant(line, vars));
        } else if (token.symbol == '(') {
            line.Next();
            sum_ = std::unique_ptr<Expression>(new Sum(line, vars));
            token = line.Get();
            if (token.type != TokenType::kSymbol || token.symbol != ')') {
                throw MatrixException("Wrong symbol");
            }
            line.Next();
        }
    }
}

std::vector<int64_t> Constant::Evaluate() {
    if (constant_) {
        coeffs_ = constant_->Evaluate();
        for (auto& coeff : coeffs_) {
            coeff *= -1;
        }
    } else if (sum_) {
        coeffs_ = sum_->Evaluate();
    }

    return coeffs_;
}

Mul::Mul(Line& line, const std::map<std::string, size_t>& vars) : Expression(vars) {
    mul_ = std::unique_ptr<Expression>(new Constant(line, vars));
    Token token = line.Get();
    while (token.type == TokenType::kSymbol && token.symbol == '*') {
        if (constant_) {
            mul_ = std::unique_ptr<Expression>(new Mul(std::move(*this)));
        }

        line.Next();
        constant_ = std::unique_ptr<Expression>(new Constant(line, vars));
        token = line.Get();
    }
}

Mul::Mul(Mul&& other)
    : Expression(other.vars_), mul_(std::move(other.mul_)), constant_(std::move(other.constant_)) {
    other.mul_ = nullptr;
    other.constant_ = nullptr;
}

std::vector<int64_t> Mul::Evaluate() {
    coeffs_ = mul_->Evaluate();
    if (constant_) {
        auto other = constant_->Evaluate();

        for (size_t i = 0; i < vars_.size(); ++i) {
            if (coeffs_[i] != 0) {
                std::swap(coeffs_, other);
                break;
            }
        }

        for (size_t i = 0; i < vars_.size(); ++i) {
            if (coeffs_[i] != 0) {
                throw MatrixException("Non constant product");
            }
        }

        for (size_t i = 0; i < coeffs_.size(); ++i) {
            coeffs_[i] = other[i] * coeffs_.back();
        }
    }
    return coeffs_;
}

Sum::Sum(Line& line, const std::map<std::string, size_t>& vars) : Expression(vars) {
    sum_ = std::unique_ptr<Expression>(new Mul(line, vars));
    Token token = line.Get();
    while (token.type == TokenType::kSymbol) {
        char symbol = token.symbol;
        if (symbol == '+' || symbol == '-') {
            if (mul_) {
                sum_ = std::unique_ptr<Expression>(new Sum(std::move(*this)));
            }

            op_ = symbol;
            line.Next();
            mul_ = std::unique_ptr<Expression>(new Mul(line, vars));
            token = line.Get();
        } else {
            break;
        }
    }
}

Sum::Sum(Sum&& other)
    : Expression(other.vars_),
      sum_(std::move(other.sum_)),
      mul_(std::move(other.mul_)),
      op_(other.op_) {
    other.sum_ = nullptr;
    other.mul_ = nullptr;
}

std::vector<int64_t> Sum::Evaluate() {
    coeffs_ = sum_->Evaluate();
    if (mul_) {
        auto other = mul_->Evaluate();
        if (op_ == '+') {
            for (size_t i = 0; i < coeffs_.size(); ++i) {
                coeffs_[i] += other[i];
            }
        } else if (op_ == '-') {
            for (size_t i = 0; i < coeffs_.size(); ++i) {
                coeffs_[i] -= other[i];
            }
        }
    }
    return coeffs_;
}
