#pragma once

#include "core/analysis.hpp"

namespace archscope::core {

double compute_abstractness(const AnalysisResult &analysis,
                            const ModuleId &module);

} // namespace archscope::core
