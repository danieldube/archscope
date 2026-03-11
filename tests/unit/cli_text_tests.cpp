#include <catch2/catch_test_macros.hpp>

#include "core/cli_text.hpp"

#include <string>

TEST_CASE("help text documents the supported bootstrap options",
          "[cli][help]") {
  const std::string help = archscope::core::help_text();

  REQUIRE(help.find("Usage: archscope <compile_commands.json> <metrics...>") !=
          std::string::npos);
  REQUIRE(help.find("--help") != std::string::npos);
  REQUIRE(help.find("--version") != std::string::npos);
  REQUIRE(help.find("--module=<kind>") != std::string::npos);
  REQUIRE(help.find("--threads=<n>") != std::string::npos);
  REQUIRE(help.find("header") != std::string::npos);
  REQUIRE(help.find("--module-filter=<text>") != std::string::npos);
  REQUIRE(help.find("abstractness") != std::string::npos);
}
