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

TEST_CASE("class-scope assembly collects unique class ids in sorted order",
          "[analysis]") {
  using archscope::core::AnalysisScope;
  using archscope::core::ClassId;
  using archscope::core::EntityDependencyGraph;
  using archscope::core::EntityId;
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Zulu"}, ModuleId{"src/zulu.cpp"}, false,
                   true, ClassId{"project::Zulu"}},
          TypeInfo{TypeId{"project::Alpha"}, ModuleId{"src/alpha.cpp"}, false,
                   true, ClassId{"project::Alpha"}},
          TypeInfo{TypeId{"project::Alpha2"}, ModuleId{"src/alpha.cpp"}, false,
                   true, ClassId{"project::Alpha"}},
      },
      EntityDependencyGraph{}, AnalysisScope::class_type);

  REQUIRE(result.classes == std::vector<ClassId>{
                                ClassId{"project::Alpha"},
                                ClassId{"project::Zulu"},
                            });
  REQUIRE(result.entities ==
          std::vector<EntityId>{
              EntityId::class_type(ClassId{"project::Alpha"}),
              EntityId::class_type(ClassId{"project::Zulu"}),
          });
}

TEST_CASE("class-scope assembly includes graph-only entities", "[analysis]") {
  using archscope::core::AnalysisScope;
  using archscope::core::ClassId;
  using archscope::core::EntityDependencyGraph;
  using archscope::core::EntityId;

  EntityDependencyGraph graph;
  graph.add_dependency(EntityId::class_type(ClassId{"project::Main"}),
                       EntityId::class_type(ClassId{"project::Api"}));

  const auto result = archscope::core::assemble_analysis_result(
      {}, std::move(graph), AnalysisScope::class_type);

  REQUIRE(result.classes == std::vector<ClassId>{
                                ClassId{"project::Api"},
                                ClassId{"project::Main"},
                            });
  REQUIRE(result.entities == std::vector<EntityId>{
                                 EntityId::class_type(ClassId{"project::Api"}),
                                 EntityId::class_type(ClassId{"project::Main"}),
                             });
}

TEST_CASE("class-scope assembly handles mixed module and class graph inputs",
          "[analysis]") {
  using archscope::core::AnalysisScope;
  using archscope::core::ClassId;
  using archscope::core::EntityDependencyGraph;
  using archscope::core::EntityId;
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  EntityDependencyGraph graph;
  graph.add_dependency(EntityId::module(ModuleId{"src/alpha.cpp"}),
                       EntityId::module(ModuleId{"src/beta.cpp"}));
  graph.add_dependency(EntityId::class_type(ClassId{"project::Zulu"}),
                       EntityId::class_type(ClassId{"project::Beta"}));

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Alpha"}, ModuleId{"src/alpha.cpp"}, false,
                   true, ClassId{"project::Alpha"}},
      },
      std::move(graph), AnalysisScope::class_type);

  REQUIRE(result.entities ==
          std::vector<EntityId>{
              EntityId::class_type(ClassId{"project::Alpha"}),
              EntityId::class_type(ClassId{"project::Beta"}),
              EntityId::class_type(ClassId{"project::Zulu"}),
          });
  REQUIRE(result.modules == std::vector<ModuleId>{ModuleId{"src/alpha.cpp"}});
}
