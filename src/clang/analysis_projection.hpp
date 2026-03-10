#pragma once

#include "core/analysis.hpp"
#include "clang/tool_runner.hpp"

namespace archscope::clang_backend {

core::AnalysisResult project_analysis(const ExtractionResult &extraction,
                                      core::ModuleKind module_kind);

} // namespace archscope::clang_backend
