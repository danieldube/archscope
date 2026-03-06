#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace archscope::core {

enum class MetricId {
    abstractness,
    instability,
    distance_from_main_sequence,
};

struct MetricValue {
    MetricId id;
    double value;
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
