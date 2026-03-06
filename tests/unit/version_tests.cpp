#include <catch2/catch_test_macros.hpp>

#include "core/version.hpp"

TEST_CASE("version string reports the bootstrap release", "[version]") {
    REQUIRE(archscope::core::tool_name() == "archscope");
    REQUIRE(archscope::core::version_string() == "archscope 0.1.0");
}
