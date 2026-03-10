#pragma once

#include "core/analysis.hpp"
#include "core/report.hpp"

#include <unordered_map>
#include <vector>

namespace archscope::core {

double compute_abstractness(const AnalysisResult &analysis,
                            const ModuleId &module);
double compute_instability(const AnalysisResult &analysis,
                           const ModuleId &module);
double compute_distance_from_main_sequence(double abstractness,
                                           double instability);
double compute_distance_from_main_sequence(const AnalysisResult &analysis,
                                           const ModuleId &module);

class MetricRegistry {
public:
  using MetricFn = double (*)(const AnalysisResult &, const ModuleId &);

  [[nodiscard]] static MetricRegistry with_defaults();

  [[nodiscard]] std::vector<MetricValue>
  compute(const AnalysisResult &analysis, const ModuleId &module,
          const std::vector<MetricId> &requested_metrics) const;

private:
  std::unordered_map<MetricId, MetricFn> registry_;
};

} // namespace archscope::core
