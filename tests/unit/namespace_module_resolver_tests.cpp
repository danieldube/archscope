#include <catch2/catch_test_macros.hpp>

#include "clang/namespace_module_resolver.hpp"

#include <clang/AST/DeclCXX.h>
#include <clang/Tooling/Tooling.h>

#include <memory>
#include <string>
#include <vector>

namespace {

const clang::CXXRecordDecl *find_record(const clang::ASTUnit &ast,
                                        const std::string &qualified_name) {
  const auto *translation_unit = ast.getASTContext().getTranslationUnitDecl();
  for (const clang::Decl *declaration : translation_unit->decls()) {
    if (const auto *record =
            llvm::dyn_cast<clang::CXXRecordDecl>(declaration)) {
      if (record->getQualifiedNameAsString() == qualified_name) {
        return record;
      }
    }

    if (const auto *namespace_decl =
            llvm::dyn_cast<clang::NamespaceDecl>(declaration)) {
      for (const clang::Decl *nested : namespace_decl->decls()) {
        if (const auto *record = llvm::dyn_cast<clang::CXXRecordDecl>(nested)) {
          if (record->getQualifiedNameAsString() == qualified_name) {
            return record;
          }
        }

        if (const auto *nested_namespace =
                llvm::dyn_cast<clang::NamespaceDecl>(nested)) {
          for (const clang::Decl *nested_decl : nested_namespace->decls()) {
            if (const auto *record =
                    llvm::dyn_cast<clang::CXXRecordDecl>(nested_decl)) {
              if (record->getQualifiedNameAsString() == qualified_name) {
                return record;
              }
            }
          }
        }
      }
    }
  }

  return nullptr;
}

std::unique_ptr<clang::ASTUnit> build_ast(const std::string &code) {
  return clang::tooling::buildASTFromCodeWithArgs(code, {"-std=c++17"},
                                                  "namespace_test.cpp");
}

} // namespace

TEST_CASE("namespace resolver maps global namespace to global module",
          "[clang][namespace]") {
  const auto ast = build_ast("struct GlobalType {};");
  REQUIRE(ast != nullptr);

  const clang::CXXRecordDecl *record = find_record(*ast, "GlobalType");
  REQUIRE(record != nullptr);

  REQUIRE(archscope::clang_backend::resolve_namespace_module(*record) ==
          "<global>");
}

TEST_CASE("namespace resolver returns fully qualified nested namespace",
          "[clang][namespace]") {
  const auto ast = build_ast("namespace a::b { struct NestedType {}; }");
  REQUIRE(ast != nullptr);

  const clang::CXXRecordDecl *record = find_record(*ast, "a::b::NestedType");
  REQUIRE(record != nullptr);

  REQUIRE(archscope::clang_backend::resolve_namespace_module(*record) ==
          "a::b");
}

TEST_CASE("namespace resolver includes anonymous namespace under its parent",
          "[clang][namespace]") {
  const auto ast =
      build_ast("namespace outer { namespace { struct Hidden {}; } }");
  REQUIRE(ast != nullptr);

  const clang::CXXRecordDecl *record =
      find_record(*ast, "outer::(anonymous namespace)::Hidden");
  REQUIRE(record != nullptr);

  REQUIRE(archscope::clang_backend::resolve_namespace_module(*record) ==
          "outer::<anonymous>");
}
