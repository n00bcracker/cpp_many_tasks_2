#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

class MatrixException : public std::runtime_error {
public:
    MatrixException(const char* msg) : std::runtime_error(msg) {
    }
};

enum TokenType { kNumber, kSymbol, kVar, kFor, kEqv, kTab, kBadToken, kNewLine, kEnd };

struct Token {
    TokenType type;
    uint64_t number;
    char symbol;
    std::string var_name;
};

class Tokenizer {
public:
    explicit Tokenizer(std::istream* in) : in_{in} {
    }

    std::vector<std::vector<Token>>& Tokenize() {
        Token token;
        std::vector<Token> cur_line;
        while (true) {
            token = Consume();
            if (token.type == TokenType::kEnd) {
                break;
            } else if (token.type == TokenType::kNewLine) {
                if (!cur_line.empty()) {
                    cur_line.push_back(token);
                    program_.emplace_back(std::move(cur_line));
                    cur_line.clear();
                }
            } else if (token.type == TokenType::kTab) {
                if (cur_line.empty() || cur_line.back().type == TokenType::kTab) {
                    cur_line.push_back(token);
                }
            } else {
                cur_line.push_back(token);
            }
        }

        if (!cur_line.empty()) {
            if (cur_line.back().type != TokenType::kNewLine) {
                auto new_line_token = Token{};
                new_line_token.type = TokenType::kNewLine;
                cur_line.push_back(new_line_token);
            }
            program_.emplace_back(std::move(cur_line));
        }

        return program_;
    }

    void PrintTokens() const {
        for (const auto& line : program_) {
            for (const auto& token : line) {
                if (token.type == TokenType::kNumber) {
                    std::cout << "kNumber(" << token.number << "), ";
                } else if (token.type == TokenType::kSymbol) {
                    std::cout << "kSymbol(" << token.symbol << "), ";
                } else if (token.type == TokenType::kVar) {
                    std::cout << "kVar(" << token.var_name << "), ";
                } else if (token.type == TokenType::kFor) {
                    std::cout << "kFor, ";
                } else if (token.type == TokenType::kEqv) {
                    std::cout << "kEqv, ";
                } else if (token.type == TokenType::kTab) {
                    std::cout << "kTab, ";
                } else if (token.type == TokenType::kBadToken) {
                    std::cout << "kBadToken, ";
                }
            }

            std::cout << std::endl;
        }
    }

private:
    Token Consume() {
        Token token;
        while (true) {
            auto symbol = in_->peek();
            if (symbol == std::char_traits<char>::eof()) {
                token.type = TokenType::kEnd;
                break;
            } else if (symbol == '\n') {
                in_->get();
                token.type = TokenType::kNewLine;
                break;
            } else if (symbol == ' ') {
                size_t whitespace_cnt = 0;
                do {
                    in_->get();
                    ++whitespace_cnt;
                    symbol = in_->peek();
                } while (symbol == ' ' && whitespace_cnt < 4);

                if (whitespace_cnt == 4) {
                    token.type = TokenType::kTab;
                    break;
                };
            } else if (kOperations.find(symbol) != std::string::npos) {
                token.type = TokenType::kSymbol;
                token.symbol = in_->get();
                break;
            } else if (symbol == '=') {
                token.type = TokenType::kEqv;
                in_->get();
                break;
            } else if (symbol >= '0' && symbol <= '9') {
                token.type = TokenType::kNumber;
                uint64_t number = 0;
                do {
                    number = number * 10 + in_->get() - '0';
                    symbol = in_->peek();
                } while (symbol >= '0' && symbol <= '9');

                token.number = number;
                break;
            } else if ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z')) {
                std::string s;
                do {
                    s.push_back(in_->get());
                    symbol = in_->peek();
                } while ((symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') ||
                         symbol == '_' || (symbol >= '0' && symbol <= '9'));

                if (s == "for") {
                    token.type = TokenType::kFor;
                } else {
                    token.type = TokenType::kVar;
                    token.var_name = std::move(s);
                }

                break;
            } else {
                token.type = TokenType::kBadToken;
                in_->get();
                break;
            }
        }

        return token;
    }

    std::istream* in_;
    std::vector<std::vector<Token>> program_;

    static constexpr std::string_view kOperations = "()+-*";
};

class Line {
public:
    Line(const std::vector<Token>& line) : iter_(line.begin()), end_(line.end()) {
    }

    const Token& Get() const {
        return *(iter_);
    }

    void Next() {
        if (iter_ == end_) {
            throw MatrixException("End line");
        }

        ++iter_;
    }

private:
    std::vector<Token>::const_iterator iter_;
    std::vector<Token>::const_iterator end_;
};
