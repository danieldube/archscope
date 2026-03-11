#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "core/metrics.hpp"

TEST_CASE("instability is zero for a module with no incoming or outgoing "
          "dependencies",
          "[metrics][instability]") {
  const archscope::core::AnalysisResult result{
      {archscope::core::ModuleId{"src/isolated.cpp"}}, {}, {}, {}, {}, {},
      archscope::core::AnalysisScope::module,
  };

  REQUIRE(archscope::core::compute_instability(
              result, archscope::core::ModuleId{"src/isolated.cpp"}) == 0.0);
}

TEST_CASE("instability is one for a module with only outgoing dependencies",
          "[metrics][instability]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::ModuleId;

  const auto result = archscope::core::assemble_analysis_result(
      {}, archscope::core::build_dependency_graph({DependencyCandidate{
              ModuleId{"src/alpha.cpp"}, ModuleId{"src/beta.cpp"}, false}}));

  REQUIRE(archscope::core::compute_instability(
              result, ModuleId{"src/alpha.cpp"}) == 1.0);
}

TEST_CASE("instability is zero for a module with only incoming dependencies",
          "[metrics][instability]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::ModuleId;

  const auto result = archscope::core::assemble_analysis_result(
      {}, archscope::core::build_dependency_graph({DependencyCandidate{
              ModuleId{"src/alpha.cpp"}, ModuleId{"src/beta.cpp"}, false}}));

  REQUIRE(archscope::core::compute_instability(
              result, ModuleId{"src/beta.cpp"}) == 0.0);
}

TEST_CASE("instability is one half when incoming and outgoing dependencies "
          "are balanced",
          "[metrics][instability]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::ModuleId;

  const auto result = archscope::core::assemble_analysis_result(
      {}, archscope::core::build_dependency_graph(
              {DependencyCandidate{ModuleId{"src/alpha.cpp"},
                                   ModuleId{"src/beta.cpp"}, false},
               DependencyCandidate{ModuleId{"src/gamma.cpp"},
                                   ModuleId{"src/alpha.cpp"}, false}}));

  REQUIRE(archscope::core::compute_instability(
              result, ModuleId{"src/alpha.cpp"}) == 0.5);
}
