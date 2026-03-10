#include "core/analysis.hpp"

#include <algorithm>
#include <utility>

namespace archscope::core {

namespace {

std::vector<ModuleId> collect_modules(const std::vector<TypeInfo> &types,
                                      const DependencyGraph &graph) {
  std::vector<ModuleId> modules;
  modules.reserve(types.size() + graph.outgoing.size());

  for (const TypeInfo &type : types) {
    modules.push_back(type.owner);
  }

  for (const auto &entry : graph.outgoing) {
    modules.push_back(entry.first);
    for (const ModuleId &target : entry.second) {
      modules.push_back(target);
    }
  }

  std::sort(modules.begin(), modules.end());
  modules.erase(std::unique(modules.begin(), modules.end()), modules.end());
  return modules;
}

} // namespace

void DependencyGraph::add_dependency(ModuleId from, ModuleId target) {
  outgoing[std::move(from)].insert(std::move(target));
}

std::vector<ModuleId> DependencyGraph::modules() const {
  return collect_modules({}, *this);
}

AnalysisResult assemble_analysis_result(std::vector<TypeInfo> types,
                                        DependencyGraph graph) {
  AnalysisResult result;
  result.modules = collect_modules(types, graph);
  result.types = std::move(types);
  result.graph = std::move(graph);
  return result;
}

DependencyGraph
build_dependency_graph(const std::vector<DependencyCandidate> &candidates) {
  DependencyGraph graph;

  for (const DependencyCandidate &candidate : candidates) {
    if (candidate.is_system || candidate.from == candidate.target) {
      continue;
    }
    graph.add_dependency(candidate.from, candidate.target);
  }

  return graph;
}

} // namespace archscope::core
