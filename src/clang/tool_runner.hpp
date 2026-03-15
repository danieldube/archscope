#pragma once

#include "common/result.hpp"
#include "core/compilation_database.hpp"

#include <string>
#include <vector>

namespace archscope::clang_backend {

struct ExtractedType {
  std::string translation_unit_path;
  std::string definition_path;
  std::string compilation_target;
  std::string namespace_module;
  std::string qualified_name;
  bool is_abstract = false;

  [[nodiscard]] bool operator==(const ExtractedType &other) const {
    return translation_unit_path == other.translation_unit_path &&
           definition_path == other.definition_path &&
           compilation_target == other.compilation_target &&
           namespace_module == other.namespace_module &&
           qualified_name == other.qualified_name &&
           is_abstract == other.is_abstract;
  }
};

struct ExtractedDependency {
  std::string from_translation_unit_path;
  std::string from_definition_path;
  std::string from_compilation_target;
  std::string from_namespace_module;
  std::string target_translation_unit_path;
  std::string target_definition_path;
  std::string target_compilation_target;
  std::string target_namespace_module;
  bool is_system = false;

  [[nodiscard]] bool operator==(const ExtractedDependency &other) const {
    return from_translation_unit_path == other.from_translation_unit_path &&
           from_definition_path == other.from_definition_path &&
           from_compilation_target == other.from_compilation_target &&
           from_namespace_module == other.from_namespace_module &&
           target_translation_unit_path == other.target_translation_unit_path &&
           target_definition_path == other.target_definition_path &&
           target_compilation_target == other.target_compilation_target &&
           target_namespace_module == other.target_namespace_module &&
           is_system == other.is_system;
  }
};

struct ExtractionResult {
  std::vector<ExtractedType> types;
  std::vector<ExtractedDependency> dependencies;
};

struct ToolRunnerError {
  std::string message;
  std::vector<std::string> failed_translation_units;
};

template <typename T> using Result = common::Result<T, ToolRunnerError>;

Result<ExtractionResult>
extract_analysis(const core::CompilationDatabase &database,
                 unsigned thread_count = 1U);

} // namespace archscope::clang_backend
