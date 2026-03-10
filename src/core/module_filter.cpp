#include "core/module_filter.hpp"

#include <filesystem>

namespace archscope::core {

namespace {

bool is_namespace_prefix_match(const std::string &module,
                               const std::string &filter) {
  return module == filter ||
         (module.size() > filter.size() && module.rfind(filter + "::", 0) == 0);
}

std::string normalize_path_string(const std::string &value) {
  return std::filesystem::path(value).lexically_normal().generic_string();
}

} // namespace

bool matches_module_filter(const ModuleKind module_kind,
                           const std::string &module,
                           const std::optional<std::string> &filter) {
  if (!filter.has_value() || filter->empty()) {
    return true;
  }

  switch (module_kind) {
  case ModuleKind::namespace_module:
    return is_namespace_prefix_match(module, *filter);
  case ModuleKind::translation_unit:
  case ModuleKind::header:
    return normalize_path_string(module).find(normalize_path_string(*filter)) !=
           std::string::npos;
  }

  return false;
}

} // namespace archscope::core
