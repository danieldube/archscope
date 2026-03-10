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

struct TypeInfo {
  TypeId id;
  ModuleId owner;
  bool is_abstract = false;
  bool is_concrete = false;

  [[nodiscard]] bool operator==(const TypeInfo &other) const {
    return id == other.id && owner == other.owner &&
           is_abstract == other.is_abstract && is_concrete == other.is_concrete;
  }
};

struct ModuleIdHash {
  std::size_t operator()(const ModuleId &module) const noexcept {
    return std::hash<std::string>{}(module.value);
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

struct AnalysisResult {
  std::vector<ModuleId> modules;
  std::vector<TypeInfo> types;
  DependencyGraph graph;
};

AnalysisResult assemble_analysis_result(std::vector<TypeInfo> types,
                                        DependencyGraph graph);
DependencyGraph
build_dependency_graph(const std::vector<DependencyCandidate> &candidates);

} // namespace archscope::core
