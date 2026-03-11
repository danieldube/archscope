#include "clang/tool_runner.hpp"
#include "clang/namespace_module_resolver.hpp"
#include "clang/parallel_executor.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
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
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace archscope::clang_backend {

namespace {

using clang::ASTContext;
using clang::CXXRecordDecl;
using clang::DiagnosticsEngine;
using clang::FieldDecl;
using clang::FunctionDecl;
using clang::NamedDecl;
using clang::ParmVarDecl;
using clang::QualType;
using clang::SourceManager;

class CountingDiagConsumer : public clang::IgnoringDiagConsumer {
public:
  void HandleDiagnostic(DiagnosticsEngine::Level level,
                        const clang::Diagnostic &) override {
    if (level == DiagnosticsEngine::Error ||
        level == DiagnosticsEngine::Fatal) {
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

struct TranslationUnitMetadata {
  std::string translation_unit_path;
  std::string compilation_target;
};

using TranslationUnitPathMap = std::map<std::string, TranslationUnitMetadata>;

std::optional<ExtractedDependency>
make_dependency(const SourceManager &source_manager,
                const TranslationUnitPathMap &translation_unit_paths,
                const std::string &translation_unit_path,
                const std::string &from_definition_path,
                const std::string &from_compilation_target,
                const std::string &from_namespace_module,
                const NamedDecl *declaration) {
  if (declaration == nullptr) {
    return std::nullopt;
  }

  const auto spelling_location =
      source_manager.getSpellingLoc(declaration->getLocation());
  if (spelling_location.isInvalid()) {
    return std::nullopt;
  }

  const auto file_name = source_manager.getFilename(spelling_location);
  if (file_name.empty()) {
    return std::nullopt;
  }

  const std::string normalized_path = normalize_path(file_name.str());
  const std::string target_namespace_module =
      resolve_namespace_module(*declaration);
  if (source_manager.isInSystemHeader(spelling_location)) {
    return ExtractedDependency{translation_unit_path,
                               from_definition_path,
                               from_compilation_target,
                               from_namespace_module,
                               normalized_path,
                               normalized_path,
                               "",
                               target_namespace_module,
                               true};
  }

  std::string target_translation_unit_path;
  std::string target_compilation_target;
  const auto found = translation_unit_paths.find(normalized_path);
  if (found != translation_unit_paths.end()) {
    target_translation_unit_path = found->second.translation_unit_path;
    target_compilation_target = found->second.compilation_target;
  }

  return ExtractedDependency{translation_unit_path,
                             from_definition_path,
                             from_compilation_target,
                             from_namespace_module,
                             target_translation_unit_path,
                             normalized_path,
                             target_compilation_target,
                             target_namespace_module,
                             false};
}

void collect_dependencies_from_type(
    QualType type, ASTContext &context,
    const TranslationUnitPathMap &translation_unit_paths,
    const std::string &translation_unit_path,
    const std::string &from_definition_path,
    const std::string &from_compilation_target,
    const std::string &from_namespace_module,
    std::vector<ExtractedDependency> &dependencies) {
  if (type.isNull()) {
    return;
  }

  if (const auto *pointer_type = type->getAs<clang::PointerType>()) {
    collect_dependencies_from_type(
        pointer_type->getPointeeType(), context, translation_unit_paths,
        translation_unit_path, from_definition_path, from_compilation_target,
        from_namespace_module, dependencies);
    return;
  }

  if (const auto *reference_type = type->getAs<clang::ReferenceType>()) {
    collect_dependencies_from_type(
        reference_type->getPointeeType(), context, translation_unit_paths,
        translation_unit_path, from_definition_path, from_compilation_target,
        from_namespace_module, dependencies);
    return;
  }

  if (const auto *array_type = context.getAsArrayType(type)) {
    collect_dependencies_from_type(
        array_type->getElementType(), context, translation_unit_paths,
        translation_unit_path, from_definition_path, from_compilation_target,
        from_namespace_module, dependencies);
    return;
  }

  if (const auto *template_type =
          type->getAs<clang::TemplateSpecializationType>()) {
    for (const clang::TemplateArgument &argument :
         template_type->template_arguments()) {
      if (argument.getKind() != clang::TemplateArgument::Type) {
        continue;
      }
      collect_dependencies_from_type(
          argument.getAsType(), context, translation_unit_paths,
          translation_unit_path, from_definition_path, from_compilation_target,
          from_namespace_module, dependencies);
    }
  }

  const clang::QualType desugared = type.getDesugaredType(context);
  if (const auto *record = desugared->getAsCXXRecordDecl()) {
    const CXXRecordDecl *definition = record->getDefinition();
    const NamedDecl *target = record;
    if (definition != nullptr) {
      target = definition;
    }
    const auto dependency =
        make_dependency(context.getSourceManager(), translation_unit_paths,
                        translation_unit_path, from_definition_path,
                        from_compilation_target, from_namespace_module, target);
    if (dependency.has_value()) {
      dependencies.push_back(*dependency);
    }
  }
}

class AnalysisCollector : public clang::RecursiveASTVisitor<AnalysisCollector> {
public:
  AnalysisCollector(ASTContext &context, std::string translation_unit_path,
                    std::string compilation_target,
                    const TranslationUnitPathMap &translation_unit_paths,
                    ExtractionResult &result)
      : context_(context),
        translation_unit_path_(std::move(translation_unit_path)),
        compilation_target_(std::move(compilation_target)),
        translation_unit_paths_(translation_unit_paths), result_(result) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *record) {
    if (!record->isThisDeclarationADefinition()) {
      return true;
    }
    if (record->isUnion()) {
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

    const std::string definition_path = normalize_path(file_name.str());
    const std::string namespace_module = resolve_namespace_module(*record);
    result_.types.push_back({translation_unit_path_, definition_path,
                             compilation_target_, namespace_module,
                             qualified_name, record->isAbstract()});

    for (const clang::CXXBaseSpecifier &base : record->bases()) {
      collect_dependencies_from_type(
          base.getType(), context_, translation_unit_paths_,
          translation_unit_path_, definition_path, compilation_target_,
          namespace_module, result_.dependencies);
    }

    for (const FieldDecl *field : record->fields()) {
      collect_dependencies_from_type(
          field->getType(), context_, translation_unit_paths_,
          translation_unit_path_, definition_path, compilation_target_,
          namespace_module, result_.dependencies);
    }
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *function) {
    if (function->isImplicit()) {
      return true;
    }

    const auto spelling_location =
        context_.getSourceManager().getSpellingLoc(function->getLocation());
    if (spelling_location.isInvalid() ||
        context_.getSourceManager().isInSystemHeader(spelling_location)) {
      return true;
    }

    const auto file_name =
        context_.getSourceManager().getFilename(spelling_location);
    if (file_name.empty()) {
      return true;
    }

    const std::string definition_path = normalize_path(file_name.str());
    const std::string namespace_module = resolve_namespace_module(*function);
    collect_dependencies_from_type(
        function->getReturnType(), context_, translation_unit_paths_,
        translation_unit_path_, definition_path, compilation_target_,
        namespace_module, result_.dependencies);

    for (const ParmVarDecl *parameter : function->parameters()) {
      collect_dependencies_from_type(
          parameter->getType(), context_, translation_unit_paths_,
          translation_unit_path_, definition_path, compilation_target_,
          namespace_module, result_.dependencies);
    }

    return true;
  }

private:
  ASTContext &context_;
  std::string translation_unit_path_;
  std::string compilation_target_;
  const TranslationUnitPathMap &translation_unit_paths_;
  ExtractionResult &result_;
};

class AnalysisCollectingConsumer : public clang::ASTConsumer {
public:
  AnalysisCollectingConsumer(
      std::string translation_unit_path, std::string compilation_target,
      const TranslationUnitPathMap &translation_unit_paths,
      ExtractionResult &result)
      : translation_unit_path_(std::move(translation_unit_path)),
        compilation_target_(std::move(compilation_target)),
        translation_unit_paths_(translation_unit_paths), result_(result) {}

  void HandleTranslationUnit(ASTContext &context) override {
    AnalysisCollector collector(context, translation_unit_path_,
                                compilation_target_, translation_unit_paths_,
                                result_);
    collector.TraverseDecl(context.getTranslationUnitDecl());
  }

private:
  std::string translation_unit_path_;
  std::string compilation_target_;
  const TranslationUnitPathMap &translation_unit_paths_;
  ExtractionResult &result_;
};

class AnalysisCollectingAction : public clang::ASTFrontendAction {
public:
  AnalysisCollectingAction(std::string translation_unit_path,
                           std::string compilation_target,
                           const TranslationUnitPathMap &translation_unit_paths,
                           ExtractionResult &result)
      : translation_unit_path_(std::move(translation_unit_path)),
        compilation_target_(std::move(compilation_target)),
        translation_unit_paths_(translation_unit_paths), result_(result) {}

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &, llvm::StringRef) override {
    return std::make_unique<AnalysisCollectingConsumer>(
        translation_unit_path_, compilation_target_, translation_unit_paths_,
        result_);
  }

private:
  std::string translation_unit_path_;
  std::string compilation_target_;
  const TranslationUnitPathMap &translation_unit_paths_;
  ExtractionResult &result_;
};

class AnalysisCollectingActionFactory
    : public clang::tooling::FrontendActionFactory {
public:
  AnalysisCollectingActionFactory(
      std::string translation_unit_path, std::string compilation_target,
      const TranslationUnitPathMap &translation_unit_paths,
      ExtractionResult &result)
      : translation_unit_path_(std::move(translation_unit_path)),
        compilation_target_(std::move(compilation_target)),
        translation_unit_paths_(translation_unit_paths), result_(result) {}

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<AnalysisCollectingAction>(
        translation_unit_path_, compilation_target_, translation_unit_paths_,
        result_);
  }

private:
  std::string translation_unit_path_;
  std::string compilation_target_;
  const TranslationUnitPathMap &translation_unit_paths_;
  ExtractionResult &result_;
};

Result<ExtractionResult>
extract_translation_unit(const core::CompilationDatabaseEntry &entry,
                         const TranslationUnitPathMap &translation_unit_paths) {
  ExtractionResult result;
  CoreCompilationDatabase compilation_database(
      core::CompilationDatabase{{entry}});
  const std::string translation_unit_path = entry.source_path;
  const std::string compilation_target = entry.compilation_target;
  const std::string resolved_translation_unit_path =
      resolve_path(entry, entry.source_path);
  clang::tooling::ClangTool tool(compilation_database,
                                 {resolved_translation_unit_path});
  CountingDiagConsumer diagnostics;
  tool.setDiagnosticConsumer(&diagnostics);
  tool.appendArgumentsAdjuster(clang::tooling::getClangSyntaxOnlyAdjuster());
  tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());

  AnalysisCollectingActionFactory factory(translation_unit_path,
                                          compilation_target,
                                          translation_unit_paths, result);
  const int tool_result = tool.run(&factory);
  if (tool_result != 0 || diagnostics.error_count() > 0U) {
    return Result<ExtractionResult>::failure(
        {"failed to parse translation unit", {translation_unit_path}});
  }

  return Result<ExtractionResult>::success(std::move(result));
}

void sort_extracted_types(std::vector<ExtractedType> &types) {
  std::sort(types.begin(), types.end(),
            [](const ExtractedType &left, const ExtractedType &right) {
              return std::tie(left.translation_unit_path, left.definition_path,
                              left.compilation_target, left.qualified_name) <
                     std::tie(right.translation_unit_path,
                              right.definition_path, right.compilation_target,
                              right.qualified_name);
            });
}

void sort_extracted_dependencies(
    std::vector<ExtractedDependency> &dependencies) {
  std::sort(
      dependencies.begin(), dependencies.end(),
      [](const ExtractedDependency &left, const ExtractedDependency &right) {
        return std::tie(left.from_translation_unit_path,
                        left.from_definition_path, left.from_compilation_target,
                        left.target_translation_unit_path,
                        left.target_definition_path, left.is_system) <
               std::tie(right.from_translation_unit_path,
                        right.from_definition_path,
                        right.from_compilation_target,
                        right.target_translation_unit_path,
                        right.target_definition_path, right.is_system);
      });
}

} // namespace

Result<ExtractionResult>
extract_analysis(const core::CompilationDatabase &database,
                 unsigned thread_count) {
  TranslationUnitPathMap translation_unit_paths;

  for (const auto &entry : database.entries) {
    translation_unit_paths.emplace(resolve_path(entry, entry.source_path),
                                   TranslationUnitMetadata{
                                       entry.source_path,
                                       entry.compilation_target,
                                   });
  }

  if (database.entries.empty()) {
    return Result<ExtractionResult>::success(ExtractionResult{});
  }

  const unsigned worker_count =
      std::max(1U, std::min(thread_count,
                            static_cast<unsigned>(database.entries.size())));
  std::vector<std::optional<ExtractionResult>> per_entry_results(
      database.entries.size());
  std::vector<std::optional<ToolRunnerError>> per_entry_errors(
      database.entries.size());
  run_in_parallel(database.entries.size(), worker_count,
                  [&database, &translation_unit_paths, &per_entry_results,
                   &per_entry_errors](const std::size_t index) {
                    const auto extracted = extract_translation_unit(
                        database.entries[index], translation_unit_paths);
                    if (extracted.has_value()) {
                      per_entry_results[index] = extracted.value();
                    } else {
                      per_entry_errors[index] = extracted.error();
                    }
                  });

  std::vector<std::string> failed_translation_units;
  for (std::size_t index = 0; index < per_entry_errors.size(); ++index) {
    if (!per_entry_errors[index].has_value()) {
      continue;
    }
    failed_translation_units.insert(
        failed_translation_units.end(),
        per_entry_errors[index]->failed_translation_units.begin(),
        per_entry_errors[index]->failed_translation_units.end());
  }
  if (!failed_translation_units.empty()) {
    return Result<ExtractionResult>::failure(
        {"failed to parse translation unit", failed_translation_units});
  }

  ExtractionResult result;
  for (std::size_t index = 0; index < per_entry_results.size(); ++index) {
    if (!per_entry_results[index].has_value()) {
      continue;
    }
    result.types.insert(result.types.end(),
                        per_entry_results[index]->types.begin(),
                        per_entry_results[index]->types.end());
    result.dependencies.insert(result.dependencies.end(),
                               per_entry_results[index]->dependencies.begin(),
                               per_entry_results[index]->dependencies.end());
  }

  sort_extracted_types(result.types);
  sort_extracted_dependencies(result.dependencies);
  return Result<ExtractionResult>::success(std::move(result));
}

} // namespace archscope::clang_backend
