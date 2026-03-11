#include <catch2/catch_test_macros.hpp>

#include "core/cli_text.hpp"

#include <string>

TEST_CASE("help text documents the supported bootstrap options",
          "[cli][help]") {
  const std::string help = archscope::core::HelpText();

  REQUIRE(help.find("Usage: archscope <compile_commands.json> <metrics...>") !=
          std::string::npos);
  REQUIRE(help.find("--help") != std::string::npos);
  REQUIRE(help.find("--version") != std::string::npos);
  REQUIRE(help.find("--module=<kind>") != std::string::npos);
  REQUIRE(help.find("--threads=<n>") != std::string::npos);
  REQUIRE(help.find("--verbose") != std::string::npos);
  REQUIRE(help.find("header") != std::string::npos);
  REQUIRE(help.find("compilation_target") != std::string::npos);
  REQUIRE(help.find("--module-filter=<text>") != std::string::npos);
  REQUIRE(help.find("abstractness") != std::string::npos);
  REQUIRE(help.find("abstract_type_count") != std::string::npos);
  REQUIRE(help.find("concrete_type_count") != std::string::npos);
  REQUIRE(help.find("type_count") != std::string::npos);
}

TEST_CASE("cli error text includes category, message, and context details",
          "[cli][error]") {
  const std::string formatted = archscope::core::FormatErrorText(
      "analysis error", "failed to parse translation unit",
      {{"translation unit", "src/broken.cpp"}, {"module", "translation_unit"}});

  REQUIRE(formatted == "error: analysis error\n"
                       "  message: failed to parse translation unit\n"
                       "  translation unit: src/broken.cpp\n"
                       "  module: translation_unit\n");
}

TEST_CASE("verbose log text uses the info prefix", "[cli][verbose]") {
  REQUIRE(archscope::core::FormatInfoText("loading compilation database") ==
          "info: loading compilation database\n");
}
