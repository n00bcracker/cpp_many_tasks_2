#include "../check_names.h"
#include "naming_rules_violation.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <clang/AST/Decl.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using clang::tooling::CommonOptionsParser;
using clang::tooling::ClangTool;
using clang::tooling::newFrontendActionFactory;
using clang::ast_matchers::MatchFinder;
using clang::ast_matchers::cxxRecordDecl;
using clang::ast_matchers::classTemplateSpecializationDecl;
using clang::ast_matchers::typeAliasDecl;
using clang::ast_matchers::typedefDecl;
using clang::ast_matchers::cxxMethodDecl;
using clang::ast_matchers::cxxConstructorDecl;
using clang::ast_matchers::cxxDestructorDecl;
using clang::ast_matchers::fieldDecl;
using clang::ast_matchers::varDecl;
using clang::ast_matchers::functionDecl;
using clang::ast_matchers::enumDecl;
using clang::ast_matchers::parmVarDecl;
using clang::ast_matchers::unless;
using clang::ast_matchers::traverse;
using clang::ast_matchers::isExpansionInSystemHeader;
using clang::ast_matchers::isImplicit;
using clang::ast_matchers::hasName;
using clang::ast_matchers::has;



static llvm::cl::OptionCategory cast_matcher_category("check-names options");
static llvm::cl::opt<std::string> spell_checker_dict_file("dict",
    llvm::cl::desc("Specify filename with spell checker dictionary"),
    llvm::cl::value_desc("path_to_dict"));

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp more_help("\nThis tool helps find violations"
                                     "of Google C++ naming rules.");

std::vector<std::string> ParseDictFile(const std::filesystem::path& filename) {
    std::ifstream in(filename);
    std::vector<std::string> res;
    std::string word;
    while (in >> word) {
        res.emplace_back(std::move(word));
    }

    return res;
}

std::unordered_map<std::string, Statistics> CheckNames(int argc, const char* argv[]) {
    auto expected_parser = CommonOptionsParser::create(argc, argv, cast_matcher_category);
    llvm::cl::ParseCommandLineOptions(argc, argv);

    std::vector<std::string> spell_checker_dict;
    if (!spell_checker_dict_file.empty()) {
        std::filesystem::path spell_checker_dict_path(spell_checker_dict_file.c_str());
        spell_checker_dict = ParseDictFile(spell_checker_dict_path);
    }

    if (!expected_parser) {
        llvm::errs() << expected_parser.takeError();
        return {};
    }

    CommonOptionsParser& options_parser = expected_parser.get();

    std::unordered_map<std::string, Statistics> res;

    for (auto& file_path : options_parser.getSourcePathList()) {
        std::filesystem::path source_path(file_path);
        ClangTool tool(options_parser.getCompilations(), file_path);

        NamingRulesViolation naming_rules_violations(spell_checker_dict);
        MatchFinder finder;

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            cxxRecordDecl(unless(isImplicit()), unless(classTemplateSpecializationDecl()), unless(isExpansionInSystemHeader())).bind("type_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            typeAliasDecl(unless(isImplicit()), unless(isExpansionInSystemHeader())).bind("type_alias_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            typedefDecl(unless(isImplicit()), unless(isExpansionInSystemHeader())).bind("type_def_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            cxxMethodDecl(unless(isImplicit()), unless(cxxConstructorDecl()), unless(cxxDestructorDecl()), unless(isExpansionInSystemHeader())).bind("method_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            cxxConstructorDecl(unless(isImplicit()), unless(isExpansionInSystemHeader())).bind("construct_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            cxxDestructorDecl(unless(isImplicit()), unless(isExpansionInSystemHeader())).bind("destruct_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            fieldDecl(unless(isExpansionInSystemHeader())).bind("field_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            varDecl(unless(parmVarDecl()), unless(isExpansionInSystemHeader())).bind("variable_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(traverse(clang::TK_IgnoreUnlessSpelledInSource,
            functionDecl(unless(cxxMethodDecl()), unless(isExpansionInSystemHeader())).bind("func_decl")),
            &naming_rules_violations
        );

        finder.addMatcher(
            enumDecl(unless(isExpansionInSystemHeader())).bind("enum_decl"),
            &naming_rules_violations
        );

        tool.run(newFrontendActionFactory(&finder).get());
        res[source_path.filename()] = naming_rules_violations.GetViolations();
    }

    return res;
}