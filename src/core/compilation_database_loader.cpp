#include "core/compilation_database.hpp"

#include "core/compile_commands_decoder.hpp"
#include "core/json_value.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace archscope::core {

Result<CompilationDatabase>
load_compilation_database(const std::filesystem::path &path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return Result<CompilationDatabase>::failure(
        {CompilationDatabaseErrorCode::file_not_found,
         "missing compile_commands.json file", path.string()});
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (input.bad()) {
    return Result<CompilationDatabase>::failure(
        {CompilationDatabaseErrorCode::io_error,
         "failed to read compile_commands.json file", path.string()});
  }

  try {
    return Result<CompilationDatabase>::success(
        decode_compile_commands(parse_json(buffer.str()), path));
  } catch (const std::runtime_error &error) {
    return Result<CompilationDatabase>::failure(
        {CompilationDatabaseErrorCode::invalid_format,
         std::string("invalid compile_commands.json: ") + error.what(),
         path.string()});
  }
}

} // namespace archscope::core
