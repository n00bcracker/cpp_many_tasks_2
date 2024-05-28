#pragma once

#include "expression.h"
#include "matrix.h"
#include "tokenizer.h"
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <utility>
#include <vector>

std::map<std::string, size_t> GatherVars(const std::vector<std::vector<Token>>& program) {
    std::map<std::string, size_t> vars;
    for (const auto& line : program) {
        for (const auto& token : line) {
            if (token.type == TokenType::kVar && !vars.contains(token.var_name)) {
                vars.emplace(token.var_name, vars.size());
            }
        }
    }

    return vars;
}

size_t CountTabs(Line& line) {
    size_t tab_count = 0;
    while (line.Get().type == TokenType::kTab) {
        ++tab_count;
        line.Next();
    }

    return tab_count;
}

class Interpreter {
public:
    Interpreter(std::vector<std::vector<Token>> program, const std::map<std::string, size_t>& vars,
                size_t depth)
        : program_(std::move(program)), vars_(vars), depth_(depth) {
    }

    Matrix CalculateTransformMatrix() {
        Matrix res = Identity(vars_.size() + 1);
        for (auto it = program_.begin(); it != program_.end();) {
            Line line(*it);
            size_t tab_count = CountTabs(line);
            if (tab_count != depth_) {
                throw MatrixException("Wrong tab count");
            }

            const Token& token = line.Get();
            if (token.type == TokenType::kVar) {
                size_t var_index = vars_.at(token.var_name);
                line.Next();

                char op = 0;

                if (line.Get().type == TokenType::kSymbol) {
                    op = line.Get().symbol;
                    if (op != '+' && op != '-' && op != '*') {
                        throw MatrixException("Wrong equation");
                    }

                    line.Next();
                }

                if (line.Get().type != TokenType::kEqv) {
                    throw MatrixException("Miss equation");
                }

                line.Next();

                auto expr_vec = Sum(line, vars_).Evaluate();
                Matrix transform = Identity(vars_.size() + 1);
                for (size_t i = 0; i < expr_vec.size(); ++i) {
                    transform(i, var_index) = expr_vec[i];
                }

                if (line.Get().type != TokenType::kNewLine) {
                    throw MatrixException("Wrong expression");
                }

                ++it;

                if (op == '+') {
                    transform(var_index, var_index) += 1;
                } else if (op == '-') {
                    for (size_t i = 0; i < vars_.size() + 1; ++i) {
                        transform(i, var_index) = -transform(i, var_index);
                    }
                    transform(var_index, var_index) += 1;
                } else if (op == '*') {
                    for (size_t i = 0; i < vars_.size(); ++i) {
                        if (transform(i, var_index) != 0) {
                            throw MatrixException("Non consntant expression for *=");
                        }
                    }

                    transform(var_index, var_index) = transform(vars_.size(), var_index);
                    transform(vars_.size(), var_index) = 0;
                }

                res *= transform;
            } else if (token.type == TokenType::kFor) {
                line.Next();
                if (line.Get().type != TokenType::kNumber) {
                    throw MatrixException("Wrong for loop format");
                }

                auto iter_count = line.Get().number;
                line.Next();

                if (line.Get().type != TokenType::kNewLine) {
                    throw MatrixException("Wrong for loop format");
                }

                ++it;
                std::vector<std::vector<Token>> subprogram;

                while (it != program_.end()) {
                    line = Line(*it);
                    if (CountTabs(line) <= depth_) {
                        break;
                    }

                    subprogram.emplace_back(std::move(*it));
                    it = program_.erase(it);
                }

                if (subprogram.empty()) {
                    throw MatrixException("Empty for loop body");
                }

                res *= Interpreter(std::move(subprogram), vars_, depth_ + 1)
                           .CalculateTransformMatrix()
                           .Pow(iter_count);
            } else if (token.type == TokenType::kNewLine) {
                ++it;
            } else {
                throw MatrixException("Wrong expression format");
            }
        }

        return res;
    }

private:
    std::vector<std::vector<Token>> program_;
    const std::map<std::string, size_t>& vars_;
    size_t depth_;
};

std::map<std::string, uint64_t> Run(std::string_view program) {
    std::istringstream ss{std::string(program)};
    Tokenizer tokenizer(&ss);
    auto lines = tokenizer.Tokenize();

    std::map<std::string, size_t> vars = GatherVars(lines);
    Matrix init_vec = Matrix(1, vars.size() + 1);
    init_vec(0, vars.size()) = 1;

    Matrix res = init_vec * Interpreter(std::move(lines), vars, 0).CalculateTransformMatrix();
    std::map<std::string, uint64_t> vars_values;

    for (const auto& [var, index] : vars) {
        vars_values.emplace(var, res(0, index));
    }

    // tokenizer.PrintTokens();
    return vars_values;
}
