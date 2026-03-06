#include <catch2/catch_test_macros.hpp>

#include "core/compilation_database.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace {

std::filesystem::path test_root() {
    const std::filesystem::path root = std::filesystem::temp_directory_path() /
                                       "archscope-tests" /
                                       "compilation_database_loader";
    std::filesystem::create_directories(root);
    return root;
}

std::filesystem::path write_fixture(const std::filesystem::path &name,
                                    std::string_view contents) {
    const std::filesystem::path path = test_root() / name;
    std::ofstream output(path);
    output << contents;
    output.close();
    return path;
}

void expect_valid_entry(const archscope::core::CompilationDatabaseEntry &entry,
                        const std::string &working_directory,
                        const std::string &source_path,
                        const std::vector<std::string> &arguments) {
    REQUIRE(entry.working_directory == working_directory);
    REQUIRE(entry.source_path == source_path);
    REQUIRE(entry.arguments.size() == arguments.size());

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        REQUIRE(entry.arguments[index] == arguments[index]);
    }
}

} // namespace

TEST_CASE("loader reads a valid compile_commands json file") {
    const std::filesystem::path db_path =
        write_fixture("valid-compile-commands.json",
                      R"json([
  {
    "directory": "/workspace/project",
    "file": "src/main.cpp",
    "arguments": [
      "clang++",
      "-std=c++17",
      "-Iinclude",
      "src/main.cpp"
    ]
  },
  {
    "directory": "/workspace/project",
    "file": "src/lib.cpp",
    "command": "clang++ -std=c++17 -Iinclude src/lib.cpp"
  }
])json");

    const auto result =
        archscope::core::load_compilation_database(db_path.string());

    REQUIRE(result.has_value());
    REQUIRE(result.value().entries.size() == 2U);
    expect_valid_entry(result.value().entries[0], "/workspace/project",
                       "src/main.cpp",
                       {"clang++", "-std=c++17", "-Iinclude", "src/main.cpp"});
    expect_valid_entry(result.value().entries[1], "/workspace/project",
                       "src/lib.cpp",
                       {"clang++", "-std=c++17", "-Iinclude", "src/lib.cpp"});
}

TEST_CASE("loader reports a missing compile_commands json file") {
    const std::filesystem::path missing_path =
        test_root() / "missing-compile-commands.json";

    const auto result =
        archscope::core::load_compilation_database(missing_path.string());

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code ==
            archscope::core::CompilationDatabaseErrorCode::file_not_found);
    REQUIRE(result.error().message.find("missing") != std::string::npos);
}

TEST_CASE("loader reports invalid json content") {
    const std::filesystem::path db_path =
        write_fixture("invalid-compile-commands.json",
                      R"json([
  {
    "directory": "/workspace/project",
    "file": "src/main.cpp",
    "arguments": ["clang++", "-std=c++17", "src/main.cpp"]
  }
)json");

    const auto result =
        archscope::core::load_compilation_database(db_path.string());

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code ==
            archscope::core::CompilationDatabaseErrorCode::invalid_format);
    REQUIRE(result.error().message.find("invalid") != std::string::npos);
}

TEST_CASE("loader extracts translation unit paths and compile arguments") {
    const std::filesystem::path db_path =
        write_fixture("compile-commands-with-splitting.json",
                      R"json([
  {
    "directory": "/workspace/project",
    "file": "src/quoted.cpp",
    "command": "clang++ '-DNAME=Arch Scope' -Iinclude src/quoted.cpp"
  },
  {
    "directory": "/workspace/project",
    "file": "src/flags.cpp",
    "arguments": [
      "clang++",
      "-DMODE=fast",
      "-Winvalid-pch",
      "src/flags.cpp"
    ]
  }
])json");

    const auto result =
        archscope::core::load_compilation_database(db_path.string());

    REQUIRE(result.has_value());
    REQUIRE(result.value().translation_unit_paths().size() == 2U);
    REQUIRE(result.value().translation_unit_paths()[0] == "src/flags.cpp");
    REQUIRE(result.value().translation_unit_paths()[1] == "src/quoted.cpp");
    REQUIRE(result.value().entries[0].arguments[1] == "-DNAME=Arch Scope");
    REQUIRE(result.value().entries[1].arguments[2] == "-Winvalid-pch");
}
