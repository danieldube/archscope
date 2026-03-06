#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"

#include <unordered_set>
#include <vector>

TEST_CASE("analysis result assembly collects unique modules in stable order",
          "[analysis]") {
  using archscope::core::DependencyGraph;
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Zulu"}, ModuleId{"src/zulu.cpp"}, false,
                   true},
          TypeInfo{TypeId{"project::Alpha"}, ModuleId{"src/alpha.cpp"}, false,
                   true},
          TypeInfo{TypeId{"project::Helper"}, ModuleId{"src/alpha.cpp"}, false,
                   true},
      },
      DependencyGraph{});

  REQUIRE(result.modules == std::vector<ModuleId>{
                                ModuleId{"src/alpha.cpp"},
                                ModuleId{"src/zulu.cpp"},
                            });
  REQUIRE(result.types.size() == 3U);
  REQUIRE(result.graph.outgoing.empty());
}

TEST_CASE("analysis result assembly includes modules introduced by dependency "
          "edges",
          "[analysis]") {
  using archscope::core::DependencyGraph;
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  DependencyGraph graph;
  graph.add_dependency(ModuleId{"src/main.cpp"}, ModuleId{"include/api.hpp"});

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Main"}, ModuleId{"src/main.cpp"}, false,
                   true},
      },
      graph);

  REQUIRE(result.modules == std::vector<ModuleId>{
                                ModuleId{"include/api.hpp"},
                                ModuleId{"src/main.cpp"},
                            });
  REQUIRE(result.graph.outgoing.size() == 1U);
  REQUIRE(result.graph.outgoing.at(ModuleId{"src/main.cpp"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"include/api.hpp"}});
}
