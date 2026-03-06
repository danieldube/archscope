#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace archscope::core {

enum class CompilationDatabaseErrorCode : std::uint8_t {
    file_not_found,
    invalid_format,
    io_error,
};

struct CompilationDatabaseError {
    CompilationDatabaseErrorCode code;
    std::string message;
    std::string context;
};

template <typename T> class Result {
  public:
    static Result success(T value) { return Result(std::move(value)); }

    static Result failure(CompilationDatabaseError error) {
        return Result(std::move(error));
    }

    [[nodiscard]] bool has_value() const { return value_.has_value(); }

    [[nodiscard]]
    const T &value() const {
        return *value_;
    }

    [[nodiscard]]
    const CompilationDatabaseError &error() const {
        return *error_;
    }

  private:
    explicit Result(T value) : value_(std::move(value)) {}

    explicit Result(CompilationDatabaseError error)
        : error_(std::move(error)) {}

    std::optional<T> value_;
    std::optional<CompilationDatabaseError> error_;
};

struct CompilationDatabaseEntry {
    std::string source_path;
    std::vector<std::string> arguments;
    std::string working_directory;
};

struct CompilationDatabase {
    std::vector<CompilationDatabaseEntry> entries;

    [[nodiscard]] std::vector<std::string> translation_unit_paths() const;
};

Result<CompilationDatabase>
load_compilation_database(const std::filesystem::path &path);

} // namespace archscope::core
