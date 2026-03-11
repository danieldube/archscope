#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace archscope::core {

enum class ModuleKind {
  namespace_module,
  translation_unit,
  header,
  compilation_target,
};

enum class AnalysisScope {
  module,
  class_type,
};

struct ModuleId {
  std::string value;

  [[nodiscard]] bool operator==(const ModuleId &other) const {
    return value == other.value;
  }
  [[nodiscard]] bool operator!=(const ModuleId &other) const {
    return !(*this == other);
  }
  [[nodiscard]] bool operator<(const ModuleId &other) const {
    return value < other.value;
  }
};

struct TypeId {
  std::string value;

  [[nodiscard]] bool operator==(const TypeId &other) const {
    return value == other.value;
  }
};

struct ClassId {
  std::string value;

  [[nodiscard]] bool operator==(const ClassId &other) const {
    return value == other.value;
  }
  [[nodiscard]] bool operator!=(const ClassId &other) const {
    return !(*this == other);
  }
  [[nodiscard]] bool operator<(const ClassId &other) const {
    return value < other.value;
  }
};

struct EntityId {
  AnalysisScope scope = AnalysisScope::module;
  std::string value;

  [[nodiscard]] static EntityId module(ModuleId id) {
    return EntityId{AnalysisScope::module, std::move(id.value)};
  }

  [[nodiscard]] static EntityId class_type(ClassId id) {
    return EntityId{AnalysisScope::class_type, std::move(id.value)};
  }

  [[nodiscard]] bool operator==(const EntityId &other) const {
    return scope == other.scope && value == other.value;
  }
  [[nodiscard]] bool operator!=(const EntityId &other) const {
    return !(*this == other);
  }
  [[nodiscard]] bool operator<(const EntityId &other) const {
    if (scope != other.scope) {
      return scope < other.scope;
    }
    return value < other.value;
  }
};

struct TypeInfo {
  TypeId id;
  ModuleId owner;
  bool is_abstract = false;
  bool is_concrete = false;
  ClassId class_id{};

  [[nodiscard]] bool operator==(const TypeInfo &other) const {
    return id == other.id && owner == other.owner &&
           is_abstract == other.is_abstract &&
           is_concrete == other.is_concrete && class_id == other.class_id;
  }
};

struct ModuleIdHash {
  std::size_t operator()(const ModuleId &module) const noexcept {
    return std::hash<std::string>{}(module.value);
  }
};

struct ClassIdHash {
  std::size_t operator()(const ClassId &class_id) const noexcept {
    return std::hash<std::string>{}(class_id.value);
  }
};

struct EntityIdHash {
  std::size_t operator()(const EntityId &entity) const noexcept {
    return std::hash<int>{}(static_cast<int>(entity.scope)) ^
           (std::hash<std::string>{}(entity.value) << 1U);
  }
};

struct DependencyCandidate {
  ModuleId from;
  ModuleId target;
  bool is_system = false;
};

struct DependencyGraph {
  std::unordered_map<ModuleId, std::unordered_set<ModuleId, ModuleIdHash>,
                     ModuleIdHash>
      outgoing;

  void add_dependency(ModuleId from, ModuleId target);
  [[nodiscard]] std::vector<ModuleId> modules() const;
};

struct EntityDependencyGraph {
  std::unordered_map<EntityId, std::unordered_set<EntityId, EntityIdHash>,
                     EntityIdHash>
      outgoing;

  void add_dependency(EntityId from, EntityId target);
  [[nodiscard]] std::vector<EntityId> entities() const;
};

struct AnalysisResult {
  std::vector<ModuleId> modules;
  std::vector<ClassId> classes;
  std::vector<EntityId> entities;
  std::vector<TypeInfo> types;
  DependencyGraph graph;
  EntityDependencyGraph entity_graph;
  AnalysisScope scope = AnalysisScope::module;
};

AnalysisResult
assemble_analysis_result(std::vector<TypeInfo> types, DependencyGraph graph,
                         AnalysisScope scope = AnalysisScope::module);
AnalysisResult assemble_analysis_result(std::vector<TypeInfo> types,
                                        EntityDependencyGraph graph,
                                        AnalysisScope scope);
DependencyGraph
build_dependency_graph(const std::vector<DependencyCandidate> &candidates);

} // namespace archscope::core
