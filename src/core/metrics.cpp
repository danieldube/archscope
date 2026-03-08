#include "core/metrics.hpp"

#include <cmath>
#include <stdexcept>

namespace archscope::core {

double compute_abstractness(const AnalysisResult &analysis,
                            const ModuleId &module) {
    std::size_t abstract_count = 0U;
    std::size_t concrete_count = 0U;

    for (const TypeInfo &type : analysis.types) {
        if (type.owner != module) {
            continue;
        }

        if (type.is_abstract) {
            ++abstract_count;
        }

        if (type.is_concrete) {
            ++concrete_count;
        }
    }

    const auto total_count = abstract_count + concrete_count;
    if (total_count == 0U) {
        return 0.0;
    }

    return static_cast<double>(abstract_count) /
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

double compute_distance_from_main_sequence(const AnalysisResult &analysis,
                                           const ModuleId &module) {
    return std::abs(compute_abstractness(analysis, module) +
                    compute_instability(analysis, module) - 1.0);
}

MetricRegistry MetricRegistry::with_defaults() {
    MetricRegistry registry;
    registry.registry_.emplace(MetricId::abstractness, &compute_abstractness);
    registry.registry_.emplace(MetricId::instability, &compute_instability);
    registry.registry_.emplace(MetricId::distance_from_main_sequence,
                               &compute_distance_from_main_sequence);
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
