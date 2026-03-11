#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "core/metrics.hpp"

TEST_CASE("abstractness is zero for an empty module",
          "[metrics][abstractness]") {
  const archscope::core::AnalysisResult result{
      {archscope::core::ModuleId{"src/empty.cpp"}},
      {},
      {},
  };

  REQUIRE(archscope::core::compute_abstractness(
              result, archscope::core::ModuleId{"src/empty.cpp"}) == 0.0);
}

TEST_CASE("abstractness is one half for one abstract and one concrete type",
          "[metrics][abstractness]") {
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Interface"}, ModuleId{"src/sample.cpp"},
                   true, false},
          TypeInfo{TypeId{"project::Implementation"},
                   ModuleId{"src/sample.cpp"}, false, true},
      },
      {});

  REQUIRE(archscope::core::compute_abstractness(
              result, ModuleId{"src/sample.cpp"}) == 0.5);
}

TEST_CASE("abstractness is one for a module with only abstract types",
          "[metrics][abstractness]") {
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Base"}, ModuleId{"include/base.hpp"}, true,
                   false},
          TypeInfo{TypeId{"project::Service"}, ModuleId{"include/base.hpp"},
                   true, false},
      },
      {});

  REQUIRE(archscope::core::compute_abstractness(
              result, ModuleId{"include/base.hpp"}) == 1.0);
}

TEST_CASE("abstractness is zero for a module with only concrete types",
          "[metrics][abstractness]") {
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Engine"}, ModuleId{"src/engine.cpp"}, false,
                   true},
          TypeInfo{TypeId{"project::Controller"}, ModuleId{"src/engine.cpp"},
                   false, true},
      },
      {});

  REQUIRE(archscope::core::compute_abstractness(
              result, ModuleId{"src/engine.cpp"}) == 0.0);
}

TEST_CASE("type count metrics distinguish abstract, concrete, and total types",
          "[metrics][counts]") {
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::AbstractBase"}, ModuleId{"src/sample.cpp"},
                   true, false},
          TypeInfo{TypeId{"project::ConcreteA"}, ModuleId{"src/sample.cpp"},
                   false, true},
          TypeInfo{TypeId{"project::ConcreteB"}, ModuleId{"src/sample.cpp"},
                   false, true},
          TypeInfo{TypeId{"project::Other"}, ModuleId{"src/other.cpp"}, true,
                   false},
      },
      {});

  REQUIRE(archscope::core::compute_abstract_type_count(
              result, ModuleId{"src/sample.cpp"}) == 1.0);
  REQUIRE(archscope::core::compute_concrete_type_count(
              result, ModuleId{"src/sample.cpp"}) == 2.0);
  REQUIRE(archscope::core::compute_type_count(
              result, ModuleId{"src/sample.cpp"}) == 3.0);
}
