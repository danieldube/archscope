#include "core/metrics.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace archscope::core {

namespace {

struct TypeCounts {
  std::size_t abstract_count = 0U;
  std::size_t concrete_count = 0U;
};

TypeCounts CollectTypeCounts(const AnalysisResult &analysis,
                             const ModuleId &module) {
  TypeCounts counts;
  for (const TypeInfo &type : analysis.types) {
    if (type.owner != module) {
      continue;
    }

    if (type.is_abstract) {
      ++counts.abstract_count;
    }

    if (type.is_concrete) {
      ++counts.concrete_count;
    }
  }

  return counts;
}

} // namespace

double compute_abstractness(const AnalysisResult &analysis,
                            const ModuleId &module) {
  const TypeCounts counts = CollectTypeCounts(analysis, module);

  const auto total_count = counts.abstract_count + counts.concrete_count;
  if (total_count == 0U) {
    return 0.0;
  }

  return static_cast<double>(counts.abstract_count) /
         static_cast<double>(total_count);
}

double compute_instability(const AnalysisResult &analysis,
                           const ModuleId &module) {
  std::size_t ce = 0U;
  const auto outgoing = analysis.graph.outgoing.find(module);
  if (outgoing != analysis.graph.outgoing.end()) {
    ce = outgoing->second.size();
  }

  std::size_t ca = 0U;
  for (const auto &entry : analysis.graph.outgoing) {
    if (entry.first == module) {
      continue;
    }

    if (entry.second.find(module) != entry.second.end()) {
      ++ca;
    }
  }

  const auto total = ce + ca;
  if (total == 0U) {
    return 0.0;
  }

  return static_cast<double>(ce) / static_cast<double>(total);
}

double compute_distance_from_main_sequence(const double abstractness,
                                           const double instability) {
  const double distance = std::abs(abstractness + instability - 1.0);
  return std::clamp(distance, 0.0, 1.0);
}

double compute_distance_from_main_sequence(const AnalysisResult &analysis,
                                           const ModuleId &module) {
  return compute_distance_from_main_sequence(
      compute_abstractness(analysis, module),
      compute_instability(analysis, module));
}

double compute_abstract_type_count(const AnalysisResult &analysis,
                                   const ModuleId &module) {
  return static_cast<double>(
      CollectTypeCounts(analysis, module).abstract_count);
}

double compute_concrete_type_count(const AnalysisResult &analysis,
                                   const ModuleId &module) {
  return static_cast<double>(
      CollectTypeCounts(analysis, module).concrete_count);
}

double compute_type_count(const AnalysisResult &analysis,
                          const ModuleId &module) {
  const TypeCounts counts = CollectTypeCounts(analysis, module);
  return static_cast<double>(counts.abstract_count + counts.concrete_count);
}

MetricRegistry MetricRegistry::with_defaults() {
  MetricRegistry registry;
  registry.registry_.emplace(MetricId::abstractness, &compute_abstractness);
  registry.registry_.emplace(MetricId::instability, &compute_instability);
  registry.registry_.emplace(MetricId::abstract_type_count,
                             &compute_abstract_type_count);
  registry.registry_.emplace(MetricId::concrete_type_count,
                             &compute_concrete_type_count);
  registry.registry_.emplace(MetricId::type_count, &compute_type_count);
  registry.registry_.emplace(MetricId::distance_from_main_sequence,
                             static_cast<MetricRegistry::MetricFn>(
                                 &compute_distance_from_main_sequence));
  return registry;
}

std::vector<MetricValue>
MetricRegistry::compute(const AnalysisResult &analysis, const ModuleId &module,
                        const std::vector<MetricId> &requested_metrics) const {
  std::vector<MetricValue> values;
  values.reserve(requested_metrics.size());

  for (const MetricId metric : requested_metrics) {
    const auto found = registry_.find(metric);
    if (found == registry_.end()) {
      throw std::invalid_argument("requested metric is not registered");
    }
    values.push_back({metric, found->second(analysis, module)});
  }

  return values;
}

} // namespace archscope::core
