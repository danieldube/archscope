#include "core/metrics.hpp"

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

  return static_cast<double>(abstract_count) / static_cast<double>(total_count);
}

} // namespace archscope::core
