#include "core/compilation_database.hpp"

#include <algorithm>

namespace archscope::core {

std::vector<std::string> CompilationDatabase::translation_unit_paths() const {
  std::vector<std::string> paths;
  paths.reserve(entries.size());

  for (const CompilationDatabaseEntry &entry : entries) {
    paths.push_back(entry.source_path);
  }

  std::sort(paths.begin(), paths.end());
  return paths;
}

} // namespace archscope::core
