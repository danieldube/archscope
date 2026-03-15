#include <catch2/catch_test_macros.hpp>

#include "core/cli_tokens.hpp"

#include <optional>

TEST_CASE("parse_metric_id resolves supported metric ids", "[cli][tokens]") {
  REQUIRE(archscope::core::parse_metric_id("abstractness") ==
          std::optional<archscope::core::MetricId>{
              archscope::core::MetricId::abstractness});
  REQUIRE(archscope::core::parse_metric_id("instability") ==
          std::optional<archscope::core::MetricId>{
              archscope::core::MetricId::instability});
  REQUIRE(archscope::core::parse_metric_id("abstract_type_count") ==
          std::optional<archscope::core::MetricId>{
              archscope::core::MetricId::abstract_type_count});
  REQUIRE(archscope::core::parse_metric_id("concrete_type_count") ==
          std::optional<archscope::core::MetricId>{
              archscope::core::MetricId::concrete_type_count});
  REQUIRE(archscope::core::parse_metric_id("type_count") ==
          std::optional<archscope::core::MetricId>{
              archscope::core::MetricId::type_count});
  REQUIRE(archscope::core::parse_metric_id("distance_from_main_sequence") ==
          std::optional<archscope::core::MetricId>{
              archscope::core::MetricId::distance_from_main_sequence});
}

TEST_CASE("parse_metric_id rejects unsupported metric ids", "[cli][tokens]") {
  REQUIRE_FALSE(archscope::core::parse_metric_id("distance").has_value());
  REQUIRE_FALSE(archscope::core::parse_metric_id("").has_value());
}

TEST_CASE("parse_module_kind resolves supported module kinds",
          "[cli][tokens]") {
  REQUIRE(archscope::core::parse_module_kind("translation_unit") ==
          std::optional<archscope::core::ModuleKind>{
              archscope::core::ModuleKind::translation_unit});
  REQUIRE(archscope::core::parse_module_kind("namespace") ==
          std::optional<archscope::core::ModuleKind>{
              archscope::core::ModuleKind::namespace_module});
  REQUIRE(archscope::core::parse_module_kind("header") ==
          std::optional<archscope::core::ModuleKind>{
              archscope::core::ModuleKind::header});
  REQUIRE(archscope::core::parse_module_kind("compilation_target") ==
          std::optional<archscope::core::ModuleKind>{
              archscope::core::ModuleKind::compilation_target});
}

TEST_CASE("parse_module_kind rejects unsupported kinds", "[cli][tokens]") {
  REQUIRE_FALSE(archscope::core::parse_module_kind("module").has_value());
  REQUIRE_FALSE(archscope::core::parse_module_kind("").has_value());
}

TEST_CASE("supported token text lists parser-backed values", "[cli][tokens]") {
  REQUIRE(archscope::core::supported_metrics_text() ==
          "abstractness, instability, abstract_type_count, "
          "concrete_type_count, type_count, distance_from_main_sequence");
  REQUIRE(archscope::core::supported_module_kinds_text() ==
          "namespace, translation_unit, header, compilation_target");
}
