#pragma once

#include "core/compilation_database.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace archscope::clang_backend {

struct ExtractedType {
  std::string translation_unit_path;
  std::string definition_path;
  std::string qualified_name;
  bool is_abstract = false;

  [[nodiscard]] bool operator==(const ExtractedType &other) const {
    return translation_unit_path == other.translation_unit_path &&
           definition_path == other.definition_path &&
           qualified_name == other.qualified_name &&
           is_abstract == other.is_abstract;
  }
};

struct ExtractedDependency {
  std::string from_translation_unit_path;
  std::string target_translation_unit_path;
  bool is_system = false;

  [[nodiscard]] bool operator==(const ExtractedDependency &other) const {
    return from_translation_unit_path == other.from_translation_unit_path &&
           target_translation_unit_path == other.target_translation_unit_path &&
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

template <typename T> class Result {
public:
  static Result success(T value) { return Result(std::move(value)); }

  static Result failure(ToolRunnerError error) {
    return Result(std::move(error));
  }

  [[nodiscard]] bool has_value() const { return value_.has_value(); }

  [[nodiscard]] const T &value() const { return *value_; }

  [[nodiscard]] const ToolRunnerError &error() const { return *error_; }

private:
  explicit Result(T value) : value_(std::move(value)) {}

  explicit Result(ToolRunnerError error) : error_(std::move(error)) {}

  std::optional<T> value_;
  std::optional<ToolRunnerError> error_;
};

Result<ExtractionResult>
extract_analysis(const core::CompilationDatabase &database);

} // namespace archscope::clang_backend
