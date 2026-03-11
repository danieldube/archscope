#include "core/report.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace archscope::core {

std::string metric_display_name(const MetricId metric_id) {
  switch (metric_id) {
  case MetricId::abstractness:
    return "Abstractness";
  case MetricId::instability:
    return "Instability";
  case MetricId::abstract_type_count:
    return "Abstract Types";
  case MetricId::concrete_type_count:
    return "Concrete Types";
  case MetricId::type_count:
    return "Types";
  case MetricId::distance_from_main_sequence:
    return "Distance from the Main Sequence";
  }

  return "Unknown Metric";
}

namespace {

bool IsCountMetric(const MetricId metric_id) {
  return metric_id == MetricId::abstract_type_count ||
         metric_id == MetricId::concrete_type_count ||
         metric_id == MetricId::type_count;
}

std::string FormatMetricValue(const MetricValue &metric) {
  std::ostringstream stream;
  if (IsCountMetric(metric.id)) {
    stream << static_cast<std::uint64_t>(metric.value);
    return stream.str();
  }

  stream << std::fixed << std::setprecision(3) << metric.value;
  return stream.str();
}

} // namespace

std::string to_markdown(const ReportModel &model) {
  std::vector<ReportModule> sorted_modules = model.modules;
  std::sort(sorted_modules.begin(), sorted_modules.end(),
            [](const ReportModule &left, const ReportModule &right) {
              return left.name < right.name;
            });

  std::ostringstream output;
  output << "**" << model.project_name << "**\n\n";

  for (std::size_t index = 0; index < sorted_modules.size(); ++index) {
    const ReportModule &module = sorted_modules[index];
    output << module.name << ":\n";
    for (const MetricValue &metric : module.metrics) {
      output << " * " << metric_display_name(metric.id) << ": "
             << FormatMetricValue(metric) << '\n';
    }
    if (index + 1U < sorted_modules.size()) {
      output << '\n';
    }
  }

  return output.str();
}

bool write_report_file(const std::filesystem::path &path,
                       const std::string &content, std::string &error_message) {
  const auto parent = path.parent_path();
  if (!parent.empty()) {
    std::error_code create_error;
    std::filesystem::create_directories(parent, create_error);
    if (create_error) {
      error_message =
          "failed to create report directory: " + create_error.message();
      return false;
    }
  }

  std::ofstream output(path);
  if (!output.is_open()) {
    error_message = "failed to open report file for writing";
    return false;
  }

  output << content;
  output.close();

  if (!output) {
    error_message = "failed to write report file";
    return false;
  }

  error_message.clear();
  return true;
}

} // namespace archscope::core
