#pragma once

#include "../check_names.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <regex>
#include <string>

using clang::ast_matchers::MatchFinder;

class NamingRulesViolation : public MatchFinder::MatchCallback {
public:
    NamingRulesViolation(const std::vector<std::string>& spellchecker_dict)
        : MatchFinder::MatchCallback(), spellchecker_dict_(spellchecker_dict) {
    }

    void run(const MatchFinder::MatchResult& result) override;

    Statistics GetViolations() const {
        return violations_;
    }

private:
    void CheckMistakes(const std::string& name, const clang::SourceManager& source_manager,
                       const clang::SourceLocation& loc);

    Statistics violations_;
    std::vector<std::string> spellchecker_dict_;
    const std::regex kTypeNamePattern = std::regex(
        R"(^([A-Z]([A-Z]{3,})?[a-z]+)([A-Z][a-z]+|[A-Z]{4,}[a-z]+)*([A-Z][a-z]+|[A-Z]{3,})?$)");
    const std::regex kConstPattern = std::regex(
        R"(^k([A-Z]([A-Z]{3,})?[a-z]+)([A-Z][a-z]+|[A-Z]{4,}[a-z]+)*([A-Z][a-z]+|[A-Z]{3,})?$)");
    const std::regex kPrivateFieldPattern = std::regex(R"(^[a-z]+(_[a-z]+)*_$)");
    const std::regex kPublicFieldPattern = std::regex(R"(^[a-z]+(_[a-z]+)*$)");
};
