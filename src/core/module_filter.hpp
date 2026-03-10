#pragma once

#include "core/analysis.hpp"

#include <optional>
#include <string>

namespace archscope::core {

bool matches_module_filter(ModuleKind module_kind, const std::string &module,
                           const std::optional<std::string> &filter);

} // namespace archscope::core
