#include "core/cli_text.hpp"
#include "core/compilation_database.hpp"
#include "core/metrics.hpp"
#include "core/report.hpp"
#include "core/version.hpp"
#include "clang/tool_runner.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

struct CliOptions {
  std::filesystem::path compile_commands_path;
  std::vector<archscope::core::MetricId> metrics;
  std::string module_kind;
  std::filesystem::path report_path = "architecture-metrics.md";
  std::optional<std::string> project_name_override;
};

bool is_option(const std::string &argument) {
  return argument.rfind("--", 0) == 0;
}

std::optional<archscope::core::MetricId>
parse_metric_id(const std::string &value) {
  if (value == "abstractness") {
    return archscope::core::MetricId::abstractness;
  }
  if (value == "instability") {
    return archscope::core::MetricId::instability;
  }
  if (value == "distance_from_main_sequence") {
    return archscope::core::MetricId::distance_from_main_sequence;
  }
  return std::nullopt;
}

std::optional<CliOptions> parse_cli(const std::vector<std::string> &args,
                                    std::string &error_message) {
  if (args.empty() || (args.size() == 1U && args.front() == "--help")) {
    std::cout << archscope::core::help_text();
    return std::nullopt;
  }

  if (args.size() == 1U && args.front() == "--version") {
    std::cout << archscope::core::version_string() << '\n';
    return std::nullopt;
  }

  if (args.size() < 3U) {
    error_message = "expected a compile_commands.json path, at least one "
                    "metric, and --module=<kind>";
    return std::nullopt;
  }

  CliOptions options;
  options.compile_commands_path = args.front();

  std::size_t index = 1U;
  for (; index < args.size(); ++index) {
    const std::string &argument = args[index];
    if (is_option(argument)) {
      break;
    }

    const auto metric = parse_metric_id(argument);
    if (!metric.has_value()) {
      error_message = "unsupported metric: " + argument;
      return std::nullopt;
    }
    options.metrics.push_back(*metric);
  }

  if (options.metrics.empty()) {
    error_message = "at least one metric is required";
    return std::nullopt;
  }

  for (; index < args.size(); ++index) {
    const std::string &argument = args[index];
    if (argument.rfind("--module=", 0) == 0) {
      options.module_kind = argument.substr(std::string("--module=").size());
      continue;
    }
    if (argument.rfind("--report=", 0) == 0) {
      options.report_path = argument.substr(std::string("--report=").size());
      continue;
    }
    if (argument.rfind("--project-name=", 0) == 0) {
      options.project_name_override =
          argument.substr(std::string("--project-name=").size());
      continue;
    }

    error_message = "unsupported option: " + argument;
    return std::nullopt;
  }

  if (options.module_kind.empty()) {
    error_message = "missing required option: --module=<kind>";
    return std::nullopt;
  }

  if (options.module_kind != "translation_unit") {
    error_message =
        "only --module=translation_unit is supported in this increment";
    return std::nullopt;
  }

  return options;
}

std::string derive_project_name(const CliOptions &options) {
  if (options.project_name_override.has_value() &&
      !options.project_name_override->empty()) {
    return *options.project_name_override;
  }

  const auto parent = options.compile_commands_path.parent_path();
  if (parent.empty()) {
    return "unknown-project";
  }

  auto name = parent.filename().string();
  if (name.empty()) {
    return "unknown-project";
  }

  return name;
}

archscope::core::AnalysisResult build_translation_unit_analysis_result(
    const std::vector<archscope::clang_backend::ExtractedType> &types) {
  std::vector<archscope::core::TypeInfo> analysis_types;
  analysis_types.reserve(types.size());

  for (const auto &type : types) {
    analysis_types.push_back(
        {archscope::core::TypeId{type.qualified_name},
         archscope::core::ModuleId{type.translation_unit_path},
         type.is_abstract, !type.is_abstract});
  }

  return archscope::core::assemble_analysis_result(std::move(analysis_types),
                                                   {});
}

int run_cli(const std::vector<std::string> &args) {
  std::string error_message;
  const auto parsed = parse_cli(args, error_message);
  if (!parsed.has_value()) {
    if (error_message.empty()) {
      return 0;
    }

    std::cerr << "error: " << error_message << "\n\n"
              << archscope::core::help_text();
    return 2;
  }

  const auto database =
      archscope::core::load_compilation_database(parsed->compile_commands_path);
  if (!database.has_value()) {
    std::cerr << "error: " << database.error().message << ": "
              << database.error().context << '\n';
    return 3;
  }

  const auto extracted_types =
      archscope::clang_backend::extract_types(database.value());
  if (!extracted_types.has_value()) {
    std::cerr << "error: " << extracted_types.error().message;
    if (!extracted_types.error().failed_translation_units.empty()) {
      std::cerr << ": "
                << extracted_types.error().failed_translation_units.front();
    }
    std::cerr << '\n';
    return 4;
  }

  const auto analysis_result =
      build_translation_unit_analysis_result(extracted_types.value());

  archscope::core::ReportModel report{
      derive_project_name(*parsed),
      {},
  };

  for (const std::string &path : database.value().translation_unit_paths()) {
    archscope::core::ReportModule module{path, {}};
    for (const auto metric : parsed->metrics) {
      double value = 0.0;
      if (metric == archscope::core::MetricId::abstractness) {
        value = archscope::core::compute_abstractness(
            analysis_result, archscope::core::ModuleId{path});
      }
      module.metrics.push_back({metric, value});
    }
    report.modules.push_back(module);
  }

  const std::string markdown = archscope::core::to_markdown(report);
  if (!archscope::core::write_report_file(parsed->report_path, markdown,
                                          error_message)) {
    std::cerr << "error: " << error_message << '\n';
    return 5;
  }

  return 0;
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    for (int index = 1; index < argc; ++index) {
      args.emplace_back(argv[index]);
    }
    return run_cli(args);
  } catch (const std::exception &error) {
    std::cerr << "internal error: " << error.what() << '\n';
    return 5;
  } catch (...) {
    std::cerr << "internal error: unexpected exception\n";
    return 5;
  }
}
