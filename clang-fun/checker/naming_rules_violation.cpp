#include "naming_rules_violation.h"
#include "string_utils.h"
#include "../check_names.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Specifiers.h>
#include <llvm-16/llvm/Support/raw_ostream.h>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <regex>
#include <string>
#include <string_view>

using clang::ASTContext;
using clang::CXXRecordDecl;
using clang::TypeAliasDecl;
using clang::TypedefDecl;
using clang::CXXMethodDecl;
using clang::CXXConstructorDecl;
using clang::CXXDestructorDecl;
using clang::FieldDecl;
using clang::VarDecl;
using clang::FunctionDecl;
using clang::EnumDecl;
using clang::AccessSpecifier;

BadName LogBadName(const clang::SourceManager& source_manager, const clang::SourceLocation& loc,
                   Entity entity_type, const std::string& name) {
    auto spel_loc = source_manager.getExpansionLoc(loc);
    std::filesystem::path filename = source_manager.getFilename(spel_loc).str();
    size_t line_number = source_manager.getExpansionLineNumber(loc);
    return {filename.filename(), name, entity_type, line_number};
}

Mistake LogMistake(const clang::SourceManager& source_manager, const clang::SourceLocation& loc,
                   const std::string& name, const std::string& wrong_word, const std::string& ok_word) {
    auto spel_loc = source_manager.getExpansionLoc(loc);
    std::filesystem::path filename = source_manager.getFilename(spel_loc).str();
    size_t line_number = source_manager.getExpansionLineNumber(loc);
    return {filename.filename(), name, wrong_word, ok_word, line_number};
}

void NamingRulesViolation::CheckMistakes(const std::string& name, const clang::SourceManager& source_manager,
                                         const clang::SourceLocation& loc) {
    if (!spellchecker_dict_.empty()) {
        auto words = SplitIntoWords(name);
        for (auto& word : words) {
            if (word.size() > 3) {
                unsigned min_dist = std::numeric_limits<unsigned>::max();
                std::optional<std::string_view> nearest_word;
                for (auto& dict_word : spellchecker_dict_) {
                    unsigned dist = LevenshteinDistance(word, dict_word);
                    if (dist < min_dist) {
                        nearest_word.emplace(std::string_view(dict_word));
                        min_dist = dist;
                    }
                }

                if (min_dist > 0 && min_dist < 4) {
                    violations_.mistakes.emplace_back(LogMistake(source_manager, loc, name, std::string(word),
                                                                 std::string(*nearest_word)));
                }
            }
        }
    }
}

void NamingRulesViolation::run(const MatchFinder::MatchResult &result) {

    ASTContext *context = result.Context;
    auto &source_manager = context->getSourceManager();

    const CXXRecordDecl *type_declaration = result.Nodes.getNodeAs<CXXRecordDecl>("type_decl");
    if (type_declaration && !source_manager.isInSystemHeader(type_declaration->getLocation())) {
        if (type_declaration->getIdentifier()) {
            auto name = type_declaration->getName().str();
            auto start_loc = type_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kType, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }
        }
    }

    const TypeAliasDecl *type_alias_declaration = result.Nodes.getNodeAs<TypeAliasDecl>("type_alias_decl");
    if (type_alias_declaration && !source_manager.isInSystemHeader(type_alias_declaration->getLocation())) {
        if (type_alias_declaration->getIdentifier()) {
            auto name = type_alias_declaration->getName().str();
            auto start_loc = type_alias_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kType, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }
        }
    }

    const TypedefDecl *type_def_declaration = result.Nodes.getNodeAs<TypedefDecl>("type_def_decl");
    if (type_def_declaration && !source_manager.isInSystemHeader(type_def_declaration->getLocation())) {
        if (type_def_declaration->getIdentifier()) {
            auto name = type_def_declaration->getName().str();
            auto start_loc = type_def_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kType, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }
        }
    }

    const CXXMethodDecl *method_declaration = result.Nodes.getNodeAs<CXXMethodDecl>("method_decl");
    if (method_declaration && !source_manager.isInSystemHeader(method_declaration->getLocation())) {
        if (method_declaration->getIdentifier() && method_declaration->getName() != "run") {
            auto name = method_declaration->getName().str();
            auto start_loc = method_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kFunction, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }
        }

        for (const auto* param_declaration : method_declaration->parameters()) {
            if (param_declaration->getIdentifier()) {
                auto name = param_declaration->getName().str();
                auto start_loc = param_declaration->getBeginLoc();
                if (param_declaration->getType().isConstQualified() || param_declaration->isConstexpr()) {
                    if (!std::regex_match(name, kConstPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kConst, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                } else {
                    if (!std::regex_match(name, kPublicFieldPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kVariable, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                }
            }
        }
    }

    const CXXConstructorDecl *constructor_declaration = result.Nodes.getNodeAs<CXXConstructorDecl>("construct_decl");
    if (constructor_declaration && !source_manager.isInSystemHeader(constructor_declaration->getLocation())) {
        if (constructor_declaration->getParent()->getIdentifier()) {
            auto name = constructor_declaration->getParent()->getName().str();
            auto start_loc = constructor_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kFunction, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }

            for (const auto* param_declaration : constructor_declaration->parameters()) {
                if (param_declaration->getIdentifier()) {
                    auto name = param_declaration->getName().str();
                    auto start_loc = param_declaration->getBeginLoc();
                    if (param_declaration->getType().isConstQualified() || param_declaration->isConstexpr()) {
                        if (!std::regex_match(name, kConstPattern)) {
                            violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kConst, name));
                        } else {
                            CheckMistakes(name, source_manager, start_loc);
                        }
                    } else {
                        if (!std::regex_match(name, kPublicFieldPattern)) {
                            violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kVariable, name));
                        } else {
                            CheckMistakes(name, source_manager, start_loc);
                        }
                    }
                }
            }
        }
    }

    const CXXDestructorDecl *destructor_declaration = result.Nodes.getNodeAs<CXXDestructorDecl>("destruct_decl");
    if (destructor_declaration && !source_manager.isInSystemHeader(destructor_declaration->getLocation())) {
        if (destructor_declaration->getParent()->getIdentifier()) {
            auto name = destructor_declaration->getParent()->getName().str();
            auto start_loc = destructor_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kFunction, "~" + name));
            } else {
                CheckMistakes("~" + name, source_manager, start_loc);
            }

            for (const auto* param_declaration : destructor_declaration->parameters()) {
                if (param_declaration->getIdentifier()) {
                    auto name = param_declaration->getName().str();
                    auto start_loc = param_declaration->getBeginLoc();
                    if (param_declaration->getType().isConstQualified() || param_declaration->isConstexpr()) {
                        if (!std::regex_match(name, kConstPattern)) {
                            violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kConst, name));
                        } else {
                            CheckMistakes(name, source_manager, start_loc);
                        }
                    } else {
                        if (!std::regex_match(name, kPublicFieldPattern)) {
                            violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kVariable, name));
                        } else {
                            CheckMistakes(name, source_manager, start_loc);
                        }
                    }
                }
            }
        }
    }

    const FieldDecl *field_declaration = result.Nodes.getNodeAs<FieldDecl>("field_decl");
    if (field_declaration && !source_manager.isInSystemHeader(field_declaration->getLocation())) {
        if (field_declaration->isCanonicalDecl() && field_declaration->getIdentifier()) {
            auto name = field_declaration->getName().str();
            auto start_loc = field_declaration->getBeginLoc();
            if (field_declaration->getType().isConstQualified()) {
                if (!std::regex_match(name, kConstPattern)) {
                    violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kConst, name));
                } else {
                    CheckMistakes(name, source_manager, start_loc);
                }
            } else {
                if (field_declaration->getAccess() == AccessSpecifier::AS_private) {
                    if (!std::regex_match(name, kPrivateFieldPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kField, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                } else if (field_declaration->getAccess() == AccessSpecifier::AS_public) {
                    if (!std::regex_match(name, kPublicFieldPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kVariable, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                }
            }
        }
    }

    const VarDecl *var_declaration = result.Nodes.getNodeAs<VarDecl>("variable_decl");
    if (var_declaration && !source_manager.isInSystemHeader(var_declaration->getLocation())) {
        if (var_declaration->getIdentifier()) {
            auto name = var_declaration->getName().str();
            auto start_loc = var_declaration->getBeginLoc();
            if (var_declaration->getType().isConstQualified() || var_declaration->isConstexpr()) {
                if (!std::regex_match(name, kConstPattern)) {
                    violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kConst, name));
                } else {
                    CheckMistakes(name, source_manager, start_loc);
                }
            } else {
                if (var_declaration->getAccess() == AccessSpecifier::AS_private) {
                    if (!std::regex_match(name, kPrivateFieldPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kField, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                } else if (var_declaration->getAccess() == AccessSpecifier::AS_public ||
                           var_declaration->getAccess() == AccessSpecifier::AS_none) {
                    if (!std::regex_match(name, kPublicFieldPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kVariable, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                }
            }
        }
    }

    const FunctionDecl *func_declaration = result.Nodes.getNodeAs<FunctionDecl>("func_decl");
    if (func_declaration && !source_manager.isInSystemHeader(func_declaration->getLocation())) {
        if (!func_declaration->isMain() && func_declaration->getIdentifier()) {
            auto name = func_declaration->getName().str();
            auto start_loc = func_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kFunction, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }
        }

        for (const auto* param_declaration : func_declaration->parameters()) {
            if (param_declaration->getIdentifier()) {
                auto name = param_declaration->getName().str();
                auto start_loc = param_declaration->getBeginLoc();
                if (param_declaration->getType().isConstQualified() || param_declaration->isConstexpr()) {
                    if (!std::regex_match(name, kConstPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kConst, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                } else {
                    if (!std::regex_match(name, kPublicFieldPattern)) {
                        violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kVariable, name));
                    } else {
                        CheckMistakes(name, source_manager, start_loc);
                    }
                }
            }
        }
    }

    const EnumDecl *enum_declaration = result.Nodes.getNodeAs<EnumDecl>("enum_decl");
    if (enum_declaration && !source_manager.isInSystemHeader(enum_declaration->getLocation())) {
        if (enum_declaration->getIdentifier()) {
            auto name = enum_declaration->getName().str();
            auto start_loc = enum_declaration->getBeginLoc();
            if (!std::regex_match(name, kTypeNamePattern)) {
                violations_.bad_names.emplace_back(LogBadName(source_manager, start_loc, Entity::kType, name));
            } else {
                CheckMistakes(name, source_manager, start_loc);
            }
        }
    }
}