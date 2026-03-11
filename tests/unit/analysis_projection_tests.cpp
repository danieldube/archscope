#include <catch2/catch_test_macros.hpp>

#include "core/analysis.hpp"
#include "clang/analysis_projection.hpp"

#include <unordered_set>
#include <vector>

TEST_CASE("analysis projection groups extracted data by namespace ownership",
          "[analysis][projection]") {
  using archscope::clang_backend::ExtractedDependency;
  using archscope::clang_backend::ExtractedType;
  using archscope::core::ModuleId;
  using archscope::core::ModuleKind;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const archscope::clang_backend::ExtractionResult extraction{
      {
          {"src/alpha.cpp", "src/alpha.cpp", "app", "a::b", "a::b::Interface",
           true},
          {"src/alpha.cpp", "src/alpha.cpp", "app", "a::b", "a::b::Concrete",
           false},
          {"src/beta.cpp", "src/beta.cpp", "domain", "a::c", "a::c::Leaf",
           false},
      },
      {
          {"src/alpha.cpp", "src/alpha.cpp", "app", "a::b", "src/beta.cpp",
           "src/beta.cpp", "domain", "a::c", false},
          {"src/alpha.cpp", "src/alpha.cpp", "app", "a::b", "src/alpha.cpp",
           "src/alpha.cpp", "app", "a::b", false},
      },
  };

  const auto analysis = archscope::clang_backend::project_analysis(
      extraction, ModuleKind::namespace_module);

  REQUIRE(analysis.modules ==
          std::vector<ModuleId>{ModuleId{"a::b"}, ModuleId{"a::c"}});
  REQUIRE(analysis.types ==
          std::vector<TypeInfo>{
              {TypeId{"a::b::Interface"}, ModuleId{"a::b"}, true, false},
              {TypeId{"a::b::Concrete"}, ModuleId{"a::b"}, false, true},
              {TypeId{"a::c::Leaf"}, ModuleId{"a::c"}, false, true},
          });
  REQUIRE(analysis.graph.outgoing.at(ModuleId{"a::b"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"a::c"}});
}

TEST_CASE("analysis projection preserves translation unit ownership",
          "[analysis][projection]") {
  using archscope::clang_backend::ExtractedDependency;
  using archscope::clang_backend::ExtractedType;
  using archscope::core::ModuleId;
  using archscope::core::ModuleKind;

  const archscope::clang_backend::ExtractionResult extraction{
      {
          {"src/alpha.cpp", "include/shared.hpp", "app", "a::b", "a::b::Shared",
           false},
          {"src/alpha.cpp", "src/alpha.cpp", "app", "a::b", "a::b::Alpha",
           false},
      },
      {
          {"src/alpha.cpp", "src/alpha.cpp", "app", "a::b", "src/beta.cpp",
           "src/beta.cpp", "domain", "a::c", false},
      },
  };

  const auto analysis = archscope::clang_backend::project_analysis(
      extraction, ModuleKind::translation_unit);

  REQUIRE(analysis.modules == std::vector<ModuleId>{ModuleId{"src/alpha.cpp"},
                                                    ModuleId{"src/beta.cpp"}});
  REQUIRE(analysis.graph.outgoing.at(ModuleId{"src/alpha.cpp"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"src/beta.cpp"}});
}

TEST_CASE("analysis projection uses definition paths for header ownership",
          "[analysis][projection]") {
  using archscope::clang_backend::ExtractedType;
  using archscope::core::ModuleId;
  using archscope::core::ModuleKind;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const archscope::clang_backend::ExtractionResult extraction{
      {
          {"src/alpha.cpp", "include/shared.hpp", "app", "sample",
           "sample::HeaderOnly", false},
          {"src/alpha.cpp", "src/alpha.cpp", "app", "sample",
           "sample::LocalType", false},
      },
      {},
  };

  const auto analysis = archscope::clang_backend::project_analysis(
      extraction, ModuleKind::header);

  REQUIRE(analysis.modules ==
          std::vector<ModuleId>{ModuleId{"include/shared.hpp"},
                                ModuleId{"src/alpha.cpp"}});
  REQUIRE(analysis.types == std::vector<TypeInfo>{
                                {TypeId{"sample::HeaderOnly"},
                                 ModuleId{"include/shared.hpp"}, false, true},
                                {TypeId{"sample::LocalType"},
                                 ModuleId{"src/alpha.cpp"}, false, true},
                            });
}

TEST_CASE("analysis projection uses definition paths for header dependencies",
          "[analysis][projection]") {
  using archscope::clang_backend::ExtractedDependency;
  using archscope::clang_backend::ExtractedType;
  using archscope::core::ModuleId;
  using archscope::core::ModuleKind;

  const archscope::clang_backend::ExtractionResult extraction{
      {
          {"src/alpha.cpp", "include/shared.hpp", "app", "sample",
           "sample::HeaderOnly", false},
          {"src/alpha.cpp", "src/alpha.cpp", "app", "sample",
           "sample::LocalType", false},
      },
      {
          {"src/alpha.cpp", "src/alpha.cpp", "app", "sample", "src/alpha.cpp",
           "include/shared.hpp", "app", "sample", false},
      },
  };

  const auto analysis = archscope::clang_backend::project_analysis(
      extraction, ModuleKind::header);

  REQUIRE(analysis.graph.outgoing.at(ModuleId{"src/alpha.cpp"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"include/shared.hpp"}});
}

TEST_CASE("analysis projection groups extracted data by compilation target "
          "ownership",
          "[analysis][projection]") {
  using archscope::clang_backend::ExtractedDependency;
  using archscope::clang_backend::ExtractedType;
  using archscope::core::ModuleId;
  using archscope::core::ModuleKind;
  using archscope::core::TypeId;
  using archscope::core::TypeInfo;

  const archscope::clang_backend::ExtractionResult extraction{
      {
          {"src/alpha.cpp", "src/alpha.cpp", "demo_app", "sample",
           "sample::Alpha", false},
          {"src/gamma.cpp", "src/gamma.cpp", "demo_app", "sample",
           "sample::Gamma", false},
          {"src/beta.cpp", "src/beta.cpp", "demo_domain", "sample",
           "sample::Beta", true},
      },
      {
          {"src/alpha.cpp", "src/alpha.cpp", "demo_app", "sample",
           "src/beta.cpp", "src/beta.cpp", "demo_domain", "sample", false},
      },
  };

  const auto analysis = archscope::clang_backend::project_analysis(
      extraction, ModuleKind::compilation_target);

  REQUIRE(analysis.modules ==
          std::vector<ModuleId>{ModuleId{"demo_app"}, ModuleId{"demo_domain"}});
  REQUIRE(analysis.types ==
          std::vector<TypeInfo>{
              {TypeId{"sample::Alpha"}, ModuleId{"demo_app"}, false, true},
              {TypeId{"sample::Gamma"}, ModuleId{"demo_app"}, false, true},
              {TypeId{"sample::Beta"}, ModuleId{"demo_domain"}, true, false},
          });
  REQUIRE(analysis.graph.outgoing.at(ModuleId{"demo_app"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"demo_domain"}});
}

TEST_CASE("analysis projection derives compilation target dependencies for "
          "header-defined types from target membership",
          "[analysis][projection]") {
  using archscope::clang_backend::ExtractedDependency;
  using archscope::clang_backend::ExtractedType;
  using archscope::core::ModuleId;
  using archscope::core::ModuleKind;

  const archscope::clang_backend::ExtractionResult extraction{
      {
          {"src/alpha.cpp", "include/shared.hpp", "demo_app", "sample",
           "sample::SharedPort", true},
          {"src/alpha.cpp", "src/alpha.cpp", "demo_app", "sample",
           "sample::Alpha", false},
          {"src/beta.cpp", "include/shared.hpp", "demo_domain", "sample",
           "sample::SharedPort", true},
          {"src/beta.cpp", "src/beta.cpp", "demo_domain", "sample",
           "sample::Beta", false},
      },
      {
          {"src/alpha.cpp", "src/alpha.cpp", "demo_app", "sample", "",
           "include/shared.hpp", "", "sample", false},
          {"src/beta.cpp", "src/beta.cpp", "demo_domain", "sample", "",
           "include/shared.hpp", "", "sample", false},
      },
  };

  const auto analysis = archscope::clang_backend::project_analysis(
      extraction, ModuleKind::compilation_target);

  REQUIRE(analysis.graph.outgoing.at(ModuleId{"demo_app"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"demo_domain"}});
  REQUIRE(analysis.graph.outgoing.at(ModuleId{"demo_domain"}) ==
          std::unordered_set<ModuleId, archscope::core::ModuleIdHash>{
              ModuleId{"demo_app"}});
}
