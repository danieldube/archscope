#include "clang/tool_runner.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace archscope::clang_backend {

namespace {

class CountingDiagConsumer : public clang::IgnoringDiagConsumer {
public:
  void HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                        const clang::Diagnostic &) override {
    if (level == clang::DiagnosticsEngine::Error ||
        level == clang::DiagnosticsEngine::Fatal) {
      ++error_count_;
    }
  }

  [[nodiscard]] unsigned error_count() const { return error_count_; }

private:
  unsigned error_count_ = 0;
};

std::string normalize_path(const std::filesystem::path &path) {
  std::error_code error;
  const auto normalized = std::filesystem::weakly_canonical(path, error);
  if (error) {
    return path.lexically_normal().string();
  }
  return normalized.lexically_normal().string();
}

std::string resolve_path(const core::CompilationDatabaseEntry &entry,
                         const std::string &path) {
  const std::filesystem::path raw_path(path);
  if (raw_path.is_absolute()) {
    return normalize_path(raw_path);
  }
  return normalize_path(std::filesystem::path(entry.working_directory) /
                        raw_path);
}

std::vector<std::string>
normalize_command_arguments(const core::CompilationDatabaseEntry &entry) {
  std::vector<std::string> normalized = entry.arguments;

  for (std::size_t index = 0; index < normalized.size(); ++index) {
    std::string &argument = normalized[index];
    if (argument == entry.source_path) {
      argument = resolve_path(entry, argument);
      continue;
    }

    if (argument == "-I" || argument == "-isystem" || argument == "-iquote") {
      if (index + 1U < normalized.size()) {
        normalized[index + 1U] = resolve_path(entry, normalized[index + 1U]);
        ++index;
      }
      continue;
    }

    for (const char *prefix : {"-I", "-isystem", "-iquote"}) {
      const std::string prefix_string(prefix);
      if (argument.rfind(prefix_string, 0) == 0 &&
          argument.size() > prefix_string.size()) {
        argument = prefix_string +
                   resolve_path(entry, argument.substr(prefix_string.size()));
        break;
      }
    }
  }

  return normalized;
}

clang::tooling::CompileCommand
to_compile_command(const core::CompilationDatabaseEntry &entry) {
  return clang::tooling::CompileCommand(entry.working_directory,
                                        resolve_path(entry, entry.source_path),
                                        normalize_command_arguments(entry), "");
}

class CoreCompilationDatabase : public clang::tooling::CompilationDatabase {
public:
  explicit CoreCompilationDatabase(core::CompilationDatabase database) {
    all_files_.reserve(database.entries.size());

    for (const auto &entry : database.entries) {
      auto command = to_compile_command(entry);
      all_files_.push_back(command.Filename);
      commands_.emplace(command.Filename, std::move(command));
    }
  }

  std::vector<clang::tooling::CompileCommand>
  getCompileCommands(llvm::StringRef file_path) const override {
    const auto range = commands_.equal_range(file_path.str());
    std::vector<clang::tooling::CompileCommand> commands;
    for (auto it = range.first; it != range.second; ++it) {
      commands.push_back(it->second);
    }
    return commands;
  }

  std::vector<std::string> getAllFiles() const override { return all_files_; }

  std::vector<clang::tooling::CompileCommand>
  getAllCompileCommands() const override {
    std::vector<clang::tooling::CompileCommand> commands;
    commands.reserve(commands_.size());
    for (const auto &entry : commands_) {
      commands.push_back(entry.second);
    }
    return commands;
  }

private:
  std::multimap<std::string, clang::tooling::CompileCommand> commands_;
  std::vector<std::string> all_files_;
};

class TypeCollector : public clang::RecursiveASTVisitor<TypeCollector> {
public:
  TypeCollector(clang::ASTContext &context, std::string translation_unit_path,
                std::vector<ExtractedType> &types)
      : context_(context),
        translation_unit_path_(std::move(translation_unit_path)),
        types_(types) {}

  bool VisitCXXRecordDecl(clang::CXXRecordDecl *record) {
    if (!record->isThisDeclarationADefinition()) {
      return true;
    }
    if (llvm::isa<clang::ClassTemplateSpecializationDecl>(record)) {
      return true;
    }

    const auto spelling_location =
        context_.getSourceManager().getSpellingLoc(record->getLocation());
    if (spelling_location.isInvalid() ||
        context_.getSourceManager().isInSystemHeader(spelling_location)) {
      return true;
    }

    const auto file_name =
        context_.getSourceManager().getFilename(spelling_location);
    if (file_name.empty()) {
      return true;
    }

    const std::string qualified_name = record->getQualifiedNameAsString();
    if (qualified_name.empty()) {
      return true;
    }

    types_.push_back({translation_unit_path_, normalize_path(file_name.str()),
                      qualified_name, record->isAbstract()});
    return true;
  }

private:
  clang::ASTContext &context_;
  std::string translation_unit_path_;
  std::vector<ExtractedType> &types_;
};

class TypeCollectingConsumer : public clang::ASTConsumer {
public:
  TypeCollectingConsumer(std::string translation_unit_path,
                         std::vector<ExtractedType> &types)
      : translation_unit_path_(std::move(translation_unit_path)),
        types_(types) {}

  void HandleTranslationUnit(clang::ASTContext &context) override {
    TypeCollector collector(context, translation_unit_path_, types_);
    collector.TraverseDecl(context.getTranslationUnitDecl());
  }

private:
  std::string translation_unit_path_;
  std::vector<ExtractedType> &types_;
};

class TypeCollectingAction : public clang::ASTFrontendAction {
public:
  TypeCollectingAction(std::string translation_unit_path,
                       std::vector<ExtractedType> &types)
      : translation_unit_path_(std::move(translation_unit_path)),
        types_(types) {}

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &, llvm::StringRef) override {
    return std::make_unique<TypeCollectingConsumer>(translation_unit_path_,
                                                    types_);
  }

private:
  std::string translation_unit_path_;
  std::vector<ExtractedType> &types_;
};

class TypeCollectingActionFactory
    : public clang::tooling::FrontendActionFactory {
public:
  TypeCollectingActionFactory(std::string translation_unit_path,
                              std::vector<ExtractedType> &types)
      : translation_unit_path_(std::move(translation_unit_path)),
        types_(types) {}

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<TypeCollectingAction>(translation_unit_path_,
                                                  types_);
  }

private:
  std::string translation_unit_path_;
  std::vector<ExtractedType> &types_;
};

void sort_extracted_types(std::vector<ExtractedType> &types) {
  std::sort(types.begin(), types.end(),
            [](const ExtractedType &left, const ExtractedType &right) {
              return std::tie(left.translation_unit_path, left.definition_path,
                              left.qualified_name) <
                     std::tie(right.translation_unit_path,
                              right.definition_path, right.qualified_name);
            });
}

} // namespace

Result<std::vector<ExtractedType>>
extract_types(const core::CompilationDatabase &database) {
  CoreCompilationDatabase compilation_database(database);
  std::vector<ExtractedType> types;

  for (const auto &entry : database.entries) {
    const std::string translation_unit_path = entry.source_path;
    const std::string resolved_translation_unit_path =
        resolve_path(entry, entry.source_path);
    clang::tooling::ClangTool tool(compilation_database,
                                   {resolved_translation_unit_path});
    CountingDiagConsumer diagnostics;
    tool.setDiagnosticConsumer(&diagnostics);
    tool.appendArgumentsAdjuster(clang::tooling::getClangSyntaxOnlyAdjuster());
    tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());

    TypeCollectingActionFactory factory(translation_unit_path, types);
    const int tool_result = tool.run(&factory);
    if (tool_result != 0 || diagnostics.error_count() > 0U) {
      return Result<std::vector<ExtractedType>>::failure(
          {"failed to parse translation unit", {translation_unit_path}});
    }
  }

  sort_extracted_types(types);
  return Result<std::vector<ExtractedType>>::success(std::move(types));
}

} // namespace archscope::clang_backend
