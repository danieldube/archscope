#include <catch2/catch_test_macros.hpp>

#include "core/compile_commands_decoder.hpp"
#include "core/json_value.hpp"

#include <filesystem>
#include <string>
#include <vector>

TEST_CASE("decoder prefers arguments array over command string",
          "[compile-commands-decoder]") {
    const archscope::core::JsonValue root = archscope::core::parse_json(
        R"json([
  {
    "directory": "/workspace/project",
    "file": "src/main.cpp",
    "arguments": ["clang++", "-c", "src/main.cpp"],
    "command": "clang++ ignored"
  }
])json");

    const archscope::core::CompilationDatabase database =
        archscope::core::decode_compile_commands(
            root, std::filesystem::path("/tmp/build/compile_commands.json"));

    REQUIRE(database.entries.size() == 1U);
    REQUIRE(database.entries[0].arguments ==
            std::vector<std::string>{"clang++", "-c", "src/main.cpp"});
}

TEST_CASE("decoder derives target from output and -o argument fallback",
          "[compile-commands-decoder]") {
    const archscope::core::JsonValue root = archscope::core::parse_json(
        R"json([
  {
    "directory": "/workspace/project",
    "file": "src/alpha.cpp",
    "arguments": ["clang++", "-c", "src/alpha.cpp"],
    "output": "CMakeFiles/domain_core.dir/src/alpha.cpp.o"
  },
  {
    "directory": "./build",
    "file": "../src/beta.cpp",
    "command": "clang++ -std=c++17 -o objects/domain/beta.o -c ../src/beta.cpp"
  }
])json");

    const archscope::core::CompilationDatabase database =
        archscope::core::decode_compile_commands(
            root, std::filesystem::path(
                      "/workspace/project/out/compile_commands.json"));

    REQUIRE(database.entries.size() == 2U);
    REQUIRE(database.entries[0].compilation_target == "domain_core");
    REQUIRE(database.entries[1].working_directory ==
            "/workspace/project/out/build");
    REQUIRE(database.entries[1].compilation_target ==
            "/workspace/project/out/build/objects/domain");
}
