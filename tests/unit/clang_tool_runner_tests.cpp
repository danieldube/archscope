#include <catch2/catch_test_macros.hpp>

#include "core/compilation_database.hpp"
#include "clang/tool_runner.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryProject {
public:
  explicit TemporaryProject(std::string directory_name)
      : root_(std::filesystem::temp_directory_path() /
              std::filesystem::path(
                  std::move(directory_name) + "-" +
                  std::to_string(std::chrono::steady_clock::now()
                                     .time_since_epoch()
                                     .count()))) {
    std::error_code cleanup_error;
    std::filesystem::remove_all(root_, cleanup_error);
    std::filesystem::create_directories(root_ / "src");
    std::filesystem::create_directories(root_ / "include");
  }

  ~TemporaryProject() {
    std::error_code cleanup_error;
    std::filesystem::remove_all(root_, cleanup_error);
  }

  [[nodiscard]] const std::filesystem::path &root() const { return root_; }

  void write_file(const std::filesystem::path &relative_path,
                  const std::string &content) const {
    const auto full_path = root_ / relative_path;
    std::filesystem::create_directories(full_path.parent_path());
    std::ofstream output(full_path);
    REQUIRE(output.is_open());
    output << content;
    REQUIRE(output.good());
  }

  void write_compile_commands(const std::string &content) const {
    write_file("compile_commands.json", content);
  }

private:
  std::filesystem::path root_;
};

archscope::core::CompilationDatabase
load_database(const TemporaryProject &project) {
  const auto database = archscope::core::load_compilation_database(
      project.root() / "compile_commands.json");
  REQUIRE(database.has_value());
  return database.value();
}

std::vector<archscope::clang_backend::ExtractedType>
extract_types(const TemporaryProject &project) {
  const auto extracted =
      archscope::clang_backend::extract_analysis(load_database(project));
  const std::string diagnostic =
      extracted.has_value()
          ? std::string{}
          : extracted.error().message + " :: " +
                (extracted.error().failed_translation_units.empty()
                     ? std::string{"<none>"}
                     : extracted.error().failed_translation_units.front());
  INFO(diagnostic);
  REQUIRE(extracted.has_value());
  return extracted.value().types;
}

archscope::clang_backend::ExtractionResult
extract_analysis(const TemporaryProject &project) {
  const auto extracted =
      archscope::clang_backend::extract_analysis(load_database(project));
  const std::string diagnostic =
      extracted.has_value()
          ? std::string{}
          : extracted.error().message + " :: " +
                (extracted.error().failed_translation_units.empty()
                     ? std::string{"<none>"}
                     : extracted.error().failed_translation_units.front());
  INFO(diagnostic);
  REQUIRE(extracted.has_value());
  return extracted.value();
}

std::string quoted_path(const std::filesystem::path &path) {
  return "\"" + path.string() + "\"";
}

} // namespace

TEST_CASE("clang tool runner extracts qualified type names and definition "
          "locations",
          "[clang][extract]") {
  TemporaryProject project("archscope-clang-tool-runner-positive");

  project.write_file("include/common.hpp", "#pragma once\n"
                                           "namespace sample {\n"
                                           "struct Shared {};\n"
                                           "}\n");
  project.write_file("src/alpha.cpp", "#include <string>\n"
                                      "#include \"../include/common.hpp\"\n"
                                      "namespace sample {\n"
                                      "union HiddenUnion {\n"
                                      "  int value;\n"
                                      "};\n"
                                      "struct Forward;\n"
                                      "class Interface {\n"
                                      "public:\n"
                                      "  virtual ~Interface() = default;\n"
                                      "  virtual void run() = 0;\n"
                                      "};\n"
                                      "class Alpha final {\n"
                                      "  Shared member;\n"
                                      "};\n"
                                      "}\n");
  project.write_file("src/zeta.cpp", "namespace sample::detail {\n"
                                     "struct Zeta {};\n"
                                     "}\n");

  project.write_compile_commands(
      "[\n"
      "  {\n"
      "    \"directory\": " +
      quoted_path(project.root()) +
      ",\n"
      "    \"file\": \"src/alpha.cpp\",\n"
      "    \"arguments\": [\"clang++\", \"-std=c++17\", \"src/alpha.cpp\"]\n"
      "  },\n"
      "  {\n"
      "    \"directory\": " +
      quoted_path(project.root()) +
      ",\n"
      "    \"file\": \"src/zeta.cpp\",\n"
      "    \"arguments\": [\"clang++\", \"-std=c++17\", \"src/zeta.cpp\"]\n"
      "  }\n"
      "]\n");

  const auto types = extract_types(project);

  REQUIRE(types == std::vector<archscope::clang_backend::ExtractedType>{
                       {
                           "src/alpha.cpp",
                           (project.root() / "include/common.hpp").string(),
                           "sample",
                           "sample::Shared",
                           false,
                       },
                       {
                           "src/alpha.cpp",
                           (project.root() / "src/alpha.cpp").string(),
                           "sample",
                           "sample::Alpha",
                           false,
                       },
                       {
                           "src/alpha.cpp",
                           (project.root() / "src/alpha.cpp").string(),
                           "sample",
                           "sample::Interface",
                           true,
                       },
                       {
                           "src/zeta.cpp",
                           (project.root() / "src/zeta.cpp").string(),
                           "sample::detail",
                           "sample::detail::Zeta",
                           false,
                       },
                   });

  REQUIRE(std::none_of(types.begin(), types.end(), [](const auto &type) {
    return type.qualified_name == "sample::HiddenUnion";
  }));
}

TEST_CASE("clang tool runner extracts translation-unit dependency candidates",
          "[clang][extract]") {
  TemporaryProject project("archscope-clang-tool-runner-dependencies");

  project.write_file("src/beta.cpp", "struct Beta {};\n");
  project.write_file("src/alpha.cpp", "#include \"beta.cpp\"\n"
                                      "struct Derived : Beta {};\n"
                                      "struct Alpha {\n"
                                      "  Beta member;\n"
                                      "  Beta make(Beta value);\n"
                                      "};\n");

  project.write_compile_commands(
      "[\n"
      "  {\n"
      "    \"directory\": " +
      quoted_path(project.root()) +
      ",\n"
      "    \"file\": \"src/alpha.cpp\",\n"
      "    \"arguments\": [\"clang++\", \"-std=c++17\", \"src/alpha.cpp\"]\n"
      "  },\n"
      "  {\n"
      "    \"directory\": " +
      quoted_path(project.root()) +
      ",\n"
      "    \"file\": \"src/beta.cpp\",\n"
      "    \"arguments\": [\"clang++\", \"-std=c++17\", \"src/beta.cpp\"]\n"
      "  }\n"
      "]\n");

  const auto analysis = extract_analysis(project);

  REQUIRE(analysis.dependencies ==
          std::vector<archscope::clang_backend::ExtractedDependency>{
              {"src/alpha.cpp", (project.root() / "src/alpha.cpp").string(),
               "<global>", "src/beta.cpp",
               (project.root() / "src/beta.cpp").string(), "<global>", false},
              {"src/alpha.cpp", (project.root() / "src/alpha.cpp").string(),
               "<global>", "src/beta.cpp",
               (project.root() / "src/beta.cpp").string(), "<global>", false},
              {"src/alpha.cpp", (project.root() / "src/alpha.cpp").string(),
               "<global>", "src/beta.cpp",
               (project.root() / "src/beta.cpp").string(), "<global>", false},
              {"src/alpha.cpp", (project.root() / "src/alpha.cpp").string(),
               "<global>", "src/beta.cpp",
               (project.root() / "src/beta.cpp").string(), "<global>", false},
          });
}

TEST_CASE("clang tool runner reports analysis failures", "[clang][extract]") {
  TemporaryProject project("archscope-clang-tool-runner-failure");

  project.write_file("src/broken.cpp", "struct Broken {\n"
                                       "  int value\n"
                                       "};\n");

  project.write_compile_commands("[\n"
                                 "  {\n"
                                 "    \"directory\": " +
                                 quoted_path(project.root()) +
                                 ",\n"
                                 "    \"file\": \"src/broken.cpp\",\n"
                                 "    \"arguments\": [\"clang++\", "
                                 "\"-std=c++17\", \"src/broken.cpp\"]\n"
                                 "  }\n"
                                 "]\n");

  const auto extracted =
      archscope::clang_backend::extract_analysis(load_database(project));

  REQUIRE_FALSE(extracted.has_value());
  REQUIRE(extracted.error().failed_translation_units ==
          std::vector<std::string>{"src/broken.cpp"});
  REQUIRE(extracted.error().message.find("failed to parse translation unit") !=
          std::string::npos);
}

TEST_CASE("clang tool runner captures header and source definition paths",
          "[clang][extract]") {
  TemporaryProject project("archscope-clang-tool-runner-header-ownership");

  project.write_file("include/shared.hpp", "#pragma once\n"
                                           "namespace sample {\n"
                                           "struct HeaderOnly {};\n"
                                           "}\n");
  project.write_file("src/alpha.cpp", "#include \"../include/shared.hpp\"\n"
                                      "namespace sample {\n"
                                      "struct SourceDefined {};\n"
                                      "}\n");

  project.write_compile_commands(
      "[\n"
      "  {\n"
      "    \"directory\": " +
      quoted_path(project.root()) +
      ",\n"
      "    \"file\": \"src/alpha.cpp\",\n"
      "    \"arguments\": [\"clang++\", \"-std=c++17\", \"src/alpha.cpp\"]\n"
      "  }\n"
      "]\n");

  const auto types = extract_types(project);

  REQUIRE(types == std::vector<archscope::clang_backend::ExtractedType>{
                       {
                           "src/alpha.cpp",
                           (project.root() / "include/shared.hpp").string(),
                           "sample",
                           "sample::HeaderOnly",
                           false,
                       },
                       {
                           "src/alpha.cpp",
                           (project.root() / "src/alpha.cpp").string(),
                           "sample",
                           "sample::SourceDefined",
                           false,
                       },
                   });
}

TEST_CASE("clang tool runner records dependency definition paths for header "
          "modules",
          "[clang][extract]") {
  TemporaryProject project("archscope-clang-tool-runner-header-dependencies");

  project.write_file("include/shared.hpp", "#pragma once\n"
                                           "namespace sample {\n"
                                           "struct HeaderOnly {};\n"
                                           "}\n");
  project.write_file("src/alpha.cpp", "#include \"../include/shared.hpp\"\n"
                                      "namespace sample {\n"
                                      "struct SourceDefined {\n"
                                      "  HeaderOnly member;\n"
                                      "};\n"
                                      "}\n");

  project.write_compile_commands(
      "[\n"
      "  {\n"
      "    \"directory\": " +
      quoted_path(project.root()) +
      ",\n"
      "    \"file\": \"src/alpha.cpp\",\n"
      "    \"arguments\": [\"clang++\", \"-std=c++17\", \"src/alpha.cpp\"]\n"
      "  }\n"
      "]\n");

  const auto analysis = extract_analysis(project);

  REQUIRE(analysis.dependencies ==
          std::vector<archscope::clang_backend::ExtractedDependency>{
              {"src/alpha.cpp", (project.root() / "src/alpha.cpp").string(),
               "sample", "", (project.root() / "include/shared.hpp").string(),
               "sample", false},
          });
}
