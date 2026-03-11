#include "clang/analysis_projection.hpp"

#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace archscope::clang_backend {

namespace {

using core::DependencyCandidate;
using core::ModuleId;
using core::ModuleKind;

core::ModuleId select_module(const ExtractedType &type,
                             const ModuleKind module_kind) {
  switch (module_kind) {
  case ModuleKind::namespace_module:
    return ModuleId{type.namespace_module};
  case ModuleKind::translation_unit:
    return ModuleId{type.translation_unit_path};
  case ModuleKind::header:
    return ModuleId{type.definition_path};
  case ModuleKind::compilation_target:
    return ModuleId{type.compilation_target};
  }

  throw std::invalid_argument("unsupported module kind");
}

std::optional<DependencyCandidate>
select_dependency(const ExtractedDependency &dependency,
                  const ModuleKind module_kind) {
  switch (module_kind) {
  case ModuleKind::namespace_module:
    return DependencyCandidate{ModuleId{dependency.from_namespace_module},
                               ModuleId{dependency.target_namespace_module},
                               dependency.is_system};
  case ModuleKind::translation_unit:
    if (dependency.from_translation_unit_path.empty() ||
        dependency.target_translation_unit_path.empty()) {
      return std::nullopt;
    }
    return DependencyCandidate{
        ModuleId{dependency.from_translation_unit_path},
        ModuleId{dependency.target_translation_unit_path},
        dependency.is_system};
  case ModuleKind::header:
    return DependencyCandidate{ModuleId{dependency.from_definition_path},
                               ModuleId{dependency.target_definition_path},
                               dependency.is_system};
  case ModuleKind::compilation_target:
    if (dependency.from_compilation_target.empty() ||
        dependency.target_compilation_target.empty()) {
      return std::nullopt;
    }
    return DependencyCandidate{ModuleId{dependency.from_compilation_target},
                               ModuleId{dependency.target_compilation_target},
                               dependency.is_system};
  }

  throw std::invalid_argument("unsupported module kind");
}

} // namespace

core::AnalysisResult project_analysis(const ExtractionResult &extraction,
                                      const core::ModuleKind module_kind) {
  std::vector<core::TypeInfo> analysis_types;
  analysis_types.reserve(extraction.types.size());

  for (const auto &type : extraction.types) {
    analysis_types.push_back({core::TypeId{type.qualified_name},
                              select_module(type, module_kind),
                              type.is_abstract, !type.is_abstract});
  }

  std::vector<DependencyCandidate> dependencies;
  dependencies.reserve(extraction.dependencies.size());

  for (const auto &dependency : extraction.dependencies) {
    const auto projected = select_dependency(dependency, module_kind);
    if (projected.has_value()) {
      dependencies.push_back(*projected);
    }
  }

  return core::assemble_analysis_result(
      std::move(analysis_types), core::build_dependency_graph(dependencies));
}

} // namespace archscope::clang_backend
