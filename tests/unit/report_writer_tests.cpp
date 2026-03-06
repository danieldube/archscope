#include <catch2/catch_test_macros.hpp>

#include "core/report.hpp"

#include <string>
#include <vector>

TEST_CASE(
    "report writer sorts modules deterministically and preserves metric order",
    "[report]") {
    archscope::core::ReportModel model{
        "SampleProject",
        {
            archscope::core::ReportModule{
                "zeta.cpp",
                {
                    {archscope::core::MetricId::instability, 0.0},
                    {archscope::core::MetricId::abstractness, 0.0},
                },
            },
            archscope::core::ReportModule{
                "alpha.cpp",
                {
                    {archscope::core::MetricId::instability, 0.0},
                    {archscope::core::MetricId::abstractness, 0.0},
                },
            },
        },
    };

    const std::string markdown = archscope::core::to_markdown(model);

    REQUIRE(markdown == "**SampleProject**\n"
                        "\n"
                        "alpha.cpp:\n"
                        " * Instability: 0.000\n"
                        " * Abstractness: 0.000\n"
                        "\n"
                        "zeta.cpp:\n"
                        " * Instability: 0.000\n"
                        " * Abstractness: 0.000\n");
}

TEST_CASE("report writer formats all placeholder metrics with fixed precision",
          "[report]") {
    archscope::core::ReportModel model{
        "FixtureProject",
        {
            archscope::core::ReportModule{
                "src/main.cpp",
                {
                    {archscope::core::MetricId::abstractness, 0.0},
                    {archscope::core::MetricId::distance_from_main_sequence,
                     1.0},
                },
            },
        },
    };

    const std::string markdown = archscope::core::to_markdown(model);

    REQUIRE(markdown.find(" * Abstractness: 0.000\n") != std::string::npos);
    REQUIRE(markdown.find(" * Distance from the Main Sequence: 1.000\n") !=
            std::string::npos);
}
