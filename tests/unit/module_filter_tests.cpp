#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "core/module_filter.hpp"

#include <optional>
#include <string>

TEST_CASE("namespace module filter matches exact namespace and nested children",
          "[module-filter]") {
  using archscope::core::ModuleKind;

  REQUIRE(archscope::core::matches_module_filter(ModuleKind::namespace_module,
                                                 "a::b", std::string{"a::b"}));
  REQUIRE(archscope::core::matches_module_filter(
      ModuleKind::namespace_module, "a::b::detail", std::string{"a::b"}));
  REQUIRE_FALSE(archscope::core::matches_module_filter(
      ModuleKind::namespace_module, "a::bc", std::string{"a::b"}));
}

TEST_CASE("module filter returns all modules when filter is omitted",
          "[module-filter]") {
  using archscope::core::ModuleKind;

  REQUIRE(archscope::core::matches_module_filter(ModuleKind::namespace_module,
                                                 "a::c", std::nullopt));
  REQUIRE(archscope::core::matches_module_filter(
      ModuleKind::translation_unit, "src/alpha.cpp", std::nullopt));
}

TEST_CASE("translation unit filter uses substring matching",
          "[module-filter]") {
  using archscope::core::ModuleKind;

  REQUIRE(archscope::core::matches_module_filter(ModuleKind::translation_unit,
                                                 "src/domain/alpha.cpp",
                                                 std::string{"domain"}));
  REQUIRE_FALSE(archscope::core::matches_module_filter(
      ModuleKind::translation_unit, "src/domain/alpha.cpp",
      std::string{"service"}));
}

TEST_CASE("header filter uses substring matching on normalized paths",
          "[module-filter]") {
  using archscope::core::ModuleKind;

  REQUIRE(archscope::core::matches_module_filter(
      ModuleKind::header, "/workspace/project/include/domain/alpha.hpp",
      std::string{"include/domain/../domain/alpha.hpp"}));
  REQUIRE_FALSE(archscope::core::matches_module_filter(
      ModuleKind::header, "/workspace/project/include/domain/alpha.hpp",
      std::string{"include/domain/beta.hpp"}));
}
