#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "core/metrics.hpp"

TEST_CASE("distance from main sequence follows |A + I - 1| for exact cases",
          "[metrics][distance]") {
  REQUIRE(archscope::core::compute_distance_from_main_sequence(0.0, 0.0) ==
          1.0);
  REQUIRE(archscope::core::compute_distance_from_main_sequence(0.5, 1.0) ==
          0.5);
  REQUIRE(archscope::core::compute_distance_from_main_sequence(1.0, 0.0) ==
          0.0);
}

TEST_CASE("distance from main sequence clamps floating arithmetic to [0, 1]",
          "[metrics][distance]") {
  REQUIRE(archscope::core::compute_distance_from_main_sequence(0.0, -0.25) ==
          1.0);
  REQUIRE(archscope::core::compute_distance_from_main_sequence(1.0, 1.25) ==
          1.0);
}

TEST_CASE("distance from main sequence uses module metrics",
          "[metrics][distance]") {
  using archscope::core::DependencyCandidate;
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto analysis = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"sample::Interface"}, ModuleId{"src/alpha.cpp"}, true,
                   false},
          TypeInfo{TypeId{"sample::Concrete"}, ModuleId{"src/alpha.cpp"}, false,
                   true},
      },
      archscope::core::build_dependency_graph({DependencyCandidate{
          ModuleId{"src/alpha.cpp"}, ModuleId{"src/beta.cpp"}, false}}));

  REQUIRE(archscope::core::compute_distance_from_main_sequence(
              analysis, ModuleId{"src/alpha.cpp"}) == 0.5);
}
