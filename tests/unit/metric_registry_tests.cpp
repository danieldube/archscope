#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "core/metrics.hpp"
#include "core/report.hpp"

#include <vector>

TEST_CASE("metric registry returns metrics in requested order",
          "[metrics][registry]") {
  using archscope::core::MetricId;
  using archscope::core::ModuleId;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const auto analysis = archscope::core::assemble_analysis_result(
      {
          TypeInfo{TypeId{"sample::Interface"}, ModuleId{"src/module.cpp"},
                   true, false},
          TypeInfo{TypeId{"sample::Concrete"}, ModuleId{"src/module.cpp"},
                   false, true},
      },
      {});

  const auto metrics = archscope::core::MetricRegistry::with_defaults().compute(
      analysis, ModuleId{"src/module.cpp"},
      {MetricId::instability, MetricId::abstractness,
       MetricId::abstract_type_count, MetricId::concrete_type_count,
       MetricId::type_count, MetricId::distance_from_main_sequence});

  REQUIRE(metrics.size() == 6U);
  REQUIRE(metrics[0].id == MetricId::instability);
  REQUIRE(metrics[0].value == 0.0);
  REQUIRE(metrics[1].id == MetricId::abstractness);
  REQUIRE(metrics[1].value == 0.5);
  REQUIRE(metrics[2].id == MetricId::abstract_type_count);
  REQUIRE(metrics[2].value == 1.0);
  REQUIRE(metrics[3].id == MetricId::concrete_type_count);
  REQUIRE(metrics[3].value == 1.0);
  REQUIRE(metrics[4].id == MetricId::type_count);
  REQUIRE(metrics[4].value == 2.0);
  REQUIRE(metrics[5].id == MetricId::distance_from_main_sequence);
  REQUIRE(metrics[5].value == 0.5);
}

TEST_CASE("metric registry computes abstractness for empty module",
          "[metrics][registry]") {
  using archscope::core::MetricId;
  using archscope::core::ModuleId;

  const archscope::core::AnalysisResult analysis{
      {ModuleId{"src/empty.cpp"}},
      {},
      {},
      {},
      {},
      {},
      archscope::core::AnalysisScope::module,
  };

  const auto metrics = archscope::core::MetricRegistry::with_defaults().compute(
      analysis, ModuleId{"src/empty.cpp"}, {MetricId::abstractness});

  REQUIRE(metrics == std::vector<archscope::core::MetricValue>{
                         {MetricId::abstractness, 0.0}});
}
