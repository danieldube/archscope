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
    std::string from_qualified_type;
    std::string target_translation_unit_path;
    std::string target_definition_path;
    std::string target_compilation_target;
    std::string target_namespace_module;
    std::string target_qualified_type;
    bool is_system = false;

    [[nodiscard]] bool operator==(const ExtractedDependency &other) const {
        return from_translation_unit_path ==
                   other.from_translation_unit_path &&
               from_definition_path == other.from_definition_path &&
               from_compilation_target == other.from_compilation_target &&
               from_namespace_module == other.from_namespace_module &&
               from_qualified_type == other.from_qualified_type &&
               target_translation_unit_path ==
                   other.target_translation_unit_path &&
               target_definition_path == other.target_definition_path &&
               target_compilation_target == other.target_compilation_target &&
               target_namespace_module == other.target_namespace_module &&
               target_qualified_type == other.target_qualified_type &&
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
extract_analysis(const core::CompilationDatabase &database,
                 unsigned thread_count = 1U);

} // namespace archscope::clang_backend
