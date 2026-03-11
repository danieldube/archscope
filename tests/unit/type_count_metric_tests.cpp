#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "core/metrics.hpp"

TEST_CASE("abstract type count is zero for an empty module",
          "[metrics][counts]") {
  const archscope::core::AnalysisResult result{
      {archscope::core::ModuleId{"src/empty.cpp"}},
      {},
      {},
  };

  REQUIRE(archscope::core::compute_abstract_type_count(
              result, archscope::core::ModuleId{"src/empty.cpp"}) == 0.0);
}

TEST_CASE("abstract type count includes only abstract types in the module",
          "[metrics][counts]") {
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::Interface"}, ModuleId{"src/sample.cpp"},
                   true, false},
          TypeInfo{TypeId{"project::Base"}, ModuleId{"src/sample.cpp"}, true,
                   false},
          TypeInfo{TypeId{"project::Concrete"}, ModuleId{"src/sample.cpp"},
                   false, true},
          TypeInfo{TypeId{"project::Other"}, ModuleId{"src/other.cpp"}, true,
                   false},
      },
      {});

  REQUIRE(archscope::core::compute_abstract_type_count(
              result, ModuleId{"src/sample.cpp"}) == 2.0);
}

TEST_CASE("concrete type count is zero for a module with no concrete types",
          "[metrics][counts]") {
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

  REQUIRE(archscope::core::compute_concrete_type_count(
              result, ModuleId{"include/base.hpp"}) == 0.0);
}

TEST_CASE("concrete type count includes only concrete types in the module",
          "[metrics][counts]") {
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto result = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"project::ConcreteA"}, ModuleId{"src/engine.cpp"},
                   false, true},
          TypeInfo{TypeId{"project::ConcreteB"}, ModuleId{"src/engine.cpp"},
                   false, true},
          TypeInfo{TypeId{"project::Interface"}, ModuleId{"src/engine.cpp"},
                   true, false},
          TypeInfo{TypeId{"project::Other"}, ModuleId{"src/other.cpp"}, false,
                   true},
      },
      {});

  REQUIRE(archscope::core::compute_concrete_type_count(
              result, ModuleId{"src/engine.cpp"}) == 2.0);
}

TEST_CASE("type count is zero for an empty module", "[metrics][counts]") {
  const archscope::core::AnalysisResult result{
      {archscope::core::ModuleId{"src/empty.cpp"}},
      {},
      {},
  };

  REQUIRE(archscope::core::compute_type_count(
              result, archscope::core::ModuleId{"src/empty.cpp"}) == 0.0);
}

TEST_CASE("type count equals abstract plus concrete types for the module",
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

  REQUIRE(archscope::core::compute_type_count(
              result, ModuleId{"src/sample.cpp"}) == 3.0);
  REQUIRE(
      archscope::core::compute_type_count(result, ModuleId{"src/sample.cpp"}) ==
      archscope::core::compute_abstract_type_count(result,
                                                   ModuleId{"src/sample.cpp"}) +
          archscope::core::compute_concrete_type_count(
              result, ModuleId{"src/sample.cpp"}));
}
