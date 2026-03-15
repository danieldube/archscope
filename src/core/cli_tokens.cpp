#include "core/cli_tokens.hpp"

#include <array>

namespace archscope::core {
namespace {

template <typename Enum> struct TokenEntry {
  std::string_view token;
  Enum value;
};

constexpr std::array<TokenEntry<MetricId>, 6> kMetricTokens{{
    {"abstractness", MetricId::abstractness},
    {"instability", MetricId::instability},
    {"abstract_type_count", MetricId::abstract_type_count},
    {"concrete_type_count", MetricId::concrete_type_count},
    {"type_count", MetricId::type_count},
    {"distance_from_main_sequence", MetricId::distance_from_main_sequence},
}};

constexpr std::array<TokenEntry<ModuleKind>, 4> kModuleKindTokens{{
    {"namespace", ModuleKind::namespace_module},
    {"translation_unit", ModuleKind::translation_unit},
    {"header", ModuleKind::header},
    {"compilation_target", ModuleKind::compilation_target},
}};

template <typename Enum, std::size_t N>
std::optional<Enum> parse_token(const std::array<TokenEntry<Enum>, N> &entries,
                                const std::string_view token) {
  for (const auto &entry : entries) {
    if (entry.token == token) {
      return entry.value;
    }
  }
  return std::nullopt;
}

template <typename Enum, std::size_t N>
std::string
supported_token_text(const std::array<TokenEntry<Enum>, N> &entries) {
  std::string text;
  for (std::size_t index = 0; index < entries.size(); ++index) {
    if (index > 0U) {
      text += ", ";
    }
    text += entries[index].token;
  }
  return text;
}

} // namespace

std::optional<MetricId> parse_metric_id(const std::string_view token) {
  return parse_token(kMetricTokens, token);
}

std::optional<ModuleKind> parse_module_kind(const std::string_view token) {
  return parse_token(kModuleKindTokens, token);
}

std::string supported_metrics_text() {
  return supported_token_text(kMetricTokens);
}

std::string supported_module_kinds_text() {
  return supported_token_text(kModuleKindTokens);
}

} // namespace archscope::core
