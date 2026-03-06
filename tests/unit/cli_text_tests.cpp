#include <catch2/catch_test_macros.hpp>

#include "core/cli_text.hpp"

#include <string>

TEST_CASE("help text documents the supported bootstrap options") {
    const std::string help = archscope::core::help_text();

    REQUIRE(help.find("Usage: archscope [--help] [--version]") !=
            std::string::npos);
    REQUIRE(help.find("--help") != std::string::npos);
    REQUIRE(help.find("--version") != std::string::npos);
}
