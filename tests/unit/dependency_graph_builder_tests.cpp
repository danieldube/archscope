#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"

#include <unordered_set>
#include <vector>

TEST_CASE("dependency graph builder deduplicates repeated edges",
          "[analysis][dependencies]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::DependencyGraph;
  using archscope::core::ModuleId;

  const DependencyGraph graph = archscope::core::build_dependency_graph({
      DependencyCandidate{ModuleId{"src/alpha.cpp"},
                          ModuleId{"include/shared.hpp"}, false},
      DependencyCandidate{ModuleId{"src/alpha.cpp"},
                          ModuleId{"include/shared.hpp"}, false},
      DependencyCandidate{ModuleId{"src/alpha.cpp"},
                          ModuleId{"include/shared.hpp"}, false},
  });

  REQUIRE(graph.outgoing.size() == 1U);
  REQUIRE(graph.outgoing.at(ModuleId{"src/alpha.cpp"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"include/shared.hpp"}});
}

TEST_CASE("dependency graph builder ignores self dependencies",
          "[analysis][dependencies]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::DependencyGraph;
  using archscope::core::ModuleId;

  const DependencyGraph graph = archscope::core::build_dependency_graph({
      DependencyCandidate{ModuleId{"src/alpha.cpp"}, ModuleId{"src/alpha.cpp"},
                          false},
  });

  REQUIRE(graph.outgoing.empty());
}

TEST_CASE("dependency graph builder ignores system dependencies",
          "[analysis][dependencies]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::DependencyGraph;
  using archscope::core::ModuleId;

  const DependencyGraph graph = archscope::core::build_dependency_graph({
      DependencyCandidate{ModuleId{"src/alpha.cpp"},
                          ModuleId{"include/project.hpp"}, false},
      DependencyCandidate{ModuleId{"src/alpha.cpp"},
                          ModuleId{"<system>/vector"}, true},
  });

  REQUIRE(graph.outgoing.size() == 1U);
  REQUIRE(graph.outgoing.at(ModuleId{"src/alpha.cpp"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"include/project.hpp"}});
}
