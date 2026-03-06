#include "core/version.hpp"

namespace archscope::core {

std::string tool_name() { return "archscope"; }

std::string version_string() { return tool_name() + " 0.1.0"; }

} // namespace archscope::core
