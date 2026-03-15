#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "core/json_value.hpp"

#include <stdexcept>
#include <string>

TEST_CASE("json parser rejects unterminated strings", "[json-value]") {
    REQUIRE_THROWS_WITH(
        archscope::core::parse_json(R"json(["unterminated])json"),
        Catch::Matchers::ContainsSubstring("unterminated string"));
}

TEST_CASE("json parser rejects unsupported tokens", "[json-value]") {
    REQUIRE_THROWS_WITH(
        archscope::core::parse_json("[1]"),
        Catch::Matchers::ContainsSubstring("unsupported json token"));
}

TEST_CASE("json parser decodes escaped characters", "[json-value]") {
    const archscope::core::JsonValue value =
        archscope::core::parse_json(R"json(["line\ntext\t\"quoted\""])json");

    const auto &values = archscope::core::as_array(value);
    REQUIRE(values.size() == 1U);
    REQUIRE(archscope::core::as_string(values[0]) == "line\ntext\t\"quoted\"");
}

TEST_CASE("json parser rejects unsupported escapes", "[json-value]") {
    REQUIRE_THROWS_WITH(
        archscope::core::parse_json(R"json(["\u0041"])json"),
        Catch::Matchers::ContainsSubstring("unsupported json escape"));
}
