#pragma once

#include "common/result.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
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

template <typename T>
using Result = common::Result<T, CompilationDatabaseError>;

struct CompilationDatabaseEntry {
  std::string source_path;
  std::vector<std::string> arguments;
  std::string working_directory;
  std::string compilation_target;
};

struct CompilationDatabase {
  std::vector<CompilationDatabaseEntry> entries;

  [[nodiscard]] std::vector<std::string> translation_unit_paths() const;
};

Result<CompilationDatabase>
load_compilation_database(const std::filesystem::path &path);

} // namespace archscope::core
