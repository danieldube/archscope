#pragma once

#include "core/compilation_database.hpp"
#include "core/json_value.hpp"

#include <filesystem>

namespace archscope::core {

CompilationDatabase
decode_compile_commands(const JsonValue &root,
                        const std::filesystem::path &database_path);

} // namespace archscope::core
