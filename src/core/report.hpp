#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace archscope::core {

enum class MetricId : std::uint8_t {
  abstractness,
  instability,
  abstract_type_count,
  concrete_type_count,
  type_count,
  distance_from_main_sequence,
};

struct MetricValue {
  MetricId id;
  double value;

  [[nodiscard]] bool operator==(const MetricValue &other) const {
    return id == other.id && value == other.value;
  }
};

struct ReportModule {
  std::string name;
  std::vector<MetricValue> metrics;
};

struct ReportModel {
  std::string project_name;
  std::vector<ReportModule> modules;
};

std::string metric_display_name(MetricId metric_id);
std::string to_markdown(const ReportModel &model);
bool write_report_file(const std::filesystem::path &path,
                       const std::string &content, std::string &error_message);

} // namespace archscope::core
