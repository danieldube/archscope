#include "clang/analysis_projection.hpp"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace archscope::clang_backend {

namespace {

using core::DependencyCandidate;
using core::ModuleId;
using core::ModuleKind;

using TargetMembershipIndex =
    std::unordered_map<std::string, std::unordered_set<std::string>>;

core::ModuleId SelectModule(const ExtractedType &type,
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

TargetMembershipIndex
BuildCompilationTargetMembershipIndex(const ExtractionResult &extraction) {
  TargetMembershipIndex index;

  for (const auto &type : extraction.types) {
    if (type.definition_path.empty() || type.compilation_target.empty()) {
      continue;
    }
    index[type.definition_path].insert(type.compilation_target);
  }

  return index;
}

std::vector<DependencyCandidate> SelectDependenciesForCompilationTarget(
    const ExtractedDependency &dependency,
    const TargetMembershipIndex &membership_index) {
  if (dependency.from_compilation_target.empty()) {
    return {};
  }

  std::unordered_set<std::string> target_memberships;
  if (!dependency.target_compilation_target.empty()) {
    target_memberships.insert(dependency.target_compilation_target);
  }

  const auto found = membership_index.find(dependency.target_definition_path);
  if (found != membership_index.end()) {
    target_memberships.insert(found->second.begin(), found->second.end());
  }

  std::vector<DependencyCandidate> candidates;
  candidates.reserve(target_memberships.size());
  for (const auto &target : target_memberships) {
    candidates.push_back({ModuleId{dependency.from_compilation_target},
                          ModuleId{target}, dependency.is_system});
  }

  return candidates;
}

std::vector<DependencyCandidate>
SelectDependency(const ExtractedDependency &dependency,
                 const ModuleKind module_kind,
                 const TargetMembershipIndex &membership_index) {
  switch (module_kind) {
  case ModuleKind::namespace_module:
    return {DependencyCandidate{ModuleId{dependency.from_namespace_module},
                                ModuleId{dependency.target_namespace_module},
                                dependency.is_system}};
  case ModuleKind::translation_unit:
    if (dependency.from_translation_unit_path.empty() ||
        dependency.target_translation_unit_path.empty()) {
      return {};
    }
    return {
        DependencyCandidate{ModuleId{dependency.from_translation_unit_path},
                            ModuleId{dependency.target_translation_unit_path},
                            dependency.is_system}};
  case ModuleKind::header:
    return {DependencyCandidate{ModuleId{dependency.from_definition_path},
                                ModuleId{dependency.target_definition_path},
                                dependency.is_system}};
  case ModuleKind::compilation_target:
    return SelectDependenciesForCompilationTarget(dependency, membership_index);
  }

  throw std::invalid_argument("unsupported module kind");
}

} // namespace

core::AnalysisResult project_analysis(const ExtractionResult &extraction,
                                      const core::ModuleKind module_kind) {
  std::vector<core::TypeInfo> analysis_types;
  analysis_types.reserve(extraction.types.size());
  const TargetMembershipIndex membership_index =
      BuildCompilationTargetMembershipIndex(extraction);

  for (const auto &type : extraction.types) {
    analysis_types.push_back({core::TypeId{type.qualified_name},
                              SelectModule(type, module_kind), type.is_abstract,
                              !type.is_abstract});
  }

  std::vector<DependencyCandidate> dependencies;
  dependencies.reserve(extraction.dependencies.size());

  for (const auto &dependency : extraction.dependencies) {
    const auto projected =
        SelectDependency(dependency, module_kind, membership_index);
    dependencies.insert(dependencies.end(), projected.begin(), projected.end());
  }

  return core::assemble_analysis_result(
      std::move(analysis_types), core::build_dependency_graph(dependencies));
}

} // namespace archscope::clang_backend
