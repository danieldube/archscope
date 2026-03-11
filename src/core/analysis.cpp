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

std::vector<ClassId> collect_classes(const std::vector<TypeInfo> &types,
                                     const EntityDependencyGraph &graph) {
  std::vector<ClassId> classes;
  classes.reserve(types.size() + graph.outgoing.size());

  for (const TypeInfo &type : types) {
    if (!type.class_id.value.empty()) {
      classes.push_back(type.class_id);
    }
  }

  for (const auto &entry : graph.outgoing) {
    if (entry.first.scope == AnalysisScope::class_type) {
      classes.push_back(ClassId{entry.first.value});
    }

    for (const EntityId &target : entry.second) {
      if (target.scope == AnalysisScope::class_type) {
        classes.push_back(ClassId{target.value});
      }
    }
  }

  std::sort(classes.begin(), classes.end());
  classes.erase(std::unique(classes.begin(), classes.end()), classes.end());
  return classes;
}

std::vector<EntityId> collect_entities(const std::vector<TypeInfo> &types,
                                       const EntityDependencyGraph &graph,
                                       AnalysisScope scope) {
  std::vector<EntityId> entities;
  entities.reserve(types.size() + graph.outgoing.size());

  for (const TypeInfo &type : types) {
    if (scope == AnalysisScope::module) {
      entities.push_back(EntityId::module(type.owner));
      continue;
    }

    if (!type.class_id.value.empty()) {
      entities.push_back(EntityId::class_type(type.class_id));
    }
  }

  for (const auto &entry : graph.outgoing) {
    if (entry.first.scope == scope) {
      entities.push_back(entry.first);
    }

    for (const EntityId &target : entry.second) {
      if (target.scope == scope) {
        entities.push_back(target);
      }
    }
  }

  std::sort(entities.begin(), entities.end());
  entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
  return entities;
}

EntityDependencyGraph collect_entity_graph(const DependencyGraph &graph) {
  EntityDependencyGraph entity_graph;
  for (const auto &entry : graph.outgoing) {
    for (const ModuleId &target : entry.second) {
      entity_graph.add_dependency(EntityId::module(entry.first),
                                  EntityId::module(target));
    }
  }
  return entity_graph;
}

} // namespace

void DependencyGraph::add_dependency(ModuleId from, ModuleId target) {
  outgoing[std::move(from)].insert(std::move(target));
}

std::vector<ModuleId> DependencyGraph::modules() const {
  return collect_modules({}, *this);
}

void EntityDependencyGraph::add_dependency(EntityId from, EntityId target) {
  outgoing[std::move(from)].insert(std::move(target));
}

std::vector<EntityId> EntityDependencyGraph::entities() const {
  std::vector<EntityId> entities;
  entities.reserve(outgoing.size());

  for (const auto &entry : outgoing) {
    entities.push_back(entry.first);
    for (const EntityId &target : entry.second) {
      entities.push_back(target);
    }
  }

  std::sort(entities.begin(), entities.end());
  entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
  return entities;
}

AnalysisResult assemble_analysis_result(std::vector<TypeInfo> types,
                                        DependencyGraph graph,
                                        AnalysisScope scope) {
  AnalysisResult result;
  result.modules = collect_modules(types, graph);
  result.entity_graph = collect_entity_graph(graph);
  result.classes = collect_classes(types, result.entity_graph);
  result.entities = collect_entities(types, result.entity_graph, scope);
  result.types = std::move(types);
  result.graph = std::move(graph);
  result.scope = scope;
  return result;
}

AnalysisResult assemble_analysis_result(std::vector<TypeInfo> types,
                                        EntityDependencyGraph graph,
                                        AnalysisScope scope) {
  AnalysisResult result;
  result.modules = collect_modules(types, {});
  result.classes = collect_classes(types, graph);
  result.entities = collect_entities(types, graph, scope);
  result.types = std::move(types);
  result.entity_graph = std::move(graph);
  result.scope = scope;
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
