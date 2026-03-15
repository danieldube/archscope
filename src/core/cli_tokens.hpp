#pragma once

#include "core/analysis.hpp"
#include "core/report.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace archscope::core {

std::optional<MetricId> parse_metric_id(std::string_view token);
std::optional<ModuleKind> parse_module_kind(std::string_view token);
std::string supported_metrics_text();
std::string supported_module_kinds_text();

} // namespace archscope::core
