#include "core/cli_text.hpp"
#include "core/cli_tokens.hpp"
#include "core/compilation_database.hpp"
#include "core/metrics.hpp"
#include "core/module_filter.hpp"
#include "core/report.hpp"
#include "core/version.hpp"
#include "clang/analysis_projection.hpp"
#include "clang/tool_runner.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

unsigned DefaultThreadCount() {
  const unsigned detected = std::thread::hardware_concurrency();
  if (detected == 0U) {
    return 1U;
  }
  return detected;
}

unsigned ClampThreadCount(const unsigned requested) {
  const unsigned maximum = DefaultThreadCount();
  if (requested == 0U) {
    return 1U;
  }
  return std::min(requested, maximum);
}

std::optional<unsigned> ParseThreadCount(const std::string &value) {
  if (value.empty()) {
    return std::nullopt;
  }

  std::size_t parsed_length = 0U;
  unsigned long requested = 0;
  try {
    requested = std::stoul(value, &parsed_length);
  } catch (const std::exception &) {
    return std::nullopt;
  }

  if (parsed_length != value.size()) {
    return std::nullopt;
  }

  return ClampThreadCount(static_cast<unsigned>(requested));
}

struct CliOptions {
  std::filesystem::path compile_commands_path;
  std::vector<archscope::core::MetricId> metrics;
  archscope::core::ModuleKind module_kind =
      archscope::core::ModuleKind::translation_unit;
  std::optional<std::string> module_filter;
  std::filesystem::path report_path = "architecture-metrics.md";
  std::optional<std::string> project_name_override;
  unsigned threads = DefaultThreadCount();
  bool verbose = false;
};

struct CliParseError {
  std::string message;
  std::vector<archscope::core::CliDetail> details;
};

CliParseError
build_parse_error(std::string message,
                  std::vector<archscope::core::CliDetail> details = {}) {
  return CliParseError{std::move(message), std::move(details)};
}

CliParseError build_parse_error(const std::string &message,
                                const std::string &detail_key,
                                const std::string &detail_value) {
  return build_parse_error(message, {{detail_key, detail_value}});
}

bool IsOption(const std::string &argument) {
  return argument.rfind("--", 0) == 0;
}

bool parse_positional_arguments(const std::vector<std::string> &args,
                                CliOptions &options, std::size_t &index,
                                CliParseError &error) {
  options.compile_commands_path = args.front();

  for (index = 1U; index < args.size(); ++index) {
    const std::string &argument = args[index];
    if (IsOption(argument)) {
      break;
    }

    const auto metric = archscope::core::parse_metric_id(argument);
    if (!metric.has_value()) {
      error = build_parse_error("unsupported metric", "metric", argument);
      return false;
    }
    options.metrics.push_back(*metric);
  }

  return true;
}

bool parse_option_argument(const std::string &argument, CliOptions &options,
                           bool &module_option_seen, CliParseError &error) {
  if (argument.rfind("--module=", 0) == 0) {
    const auto module_kind = archscope::core::parse_module_kind(
        argument.substr(std::string("--module=").size()));
    if (!module_kind.has_value()) {
      error = build_parse_error("unsupported module kind", "option", argument);
      return false;
    }
    options.module_kind = *module_kind;
    module_option_seen = true;
    return true;
  }

  if (argument.rfind("--report=", 0) == 0) {
    options.report_path = argument.substr(std::string("--report=").size());
    return true;
  }

  if (argument.rfind("--project-name=", 0) == 0) {
    options.project_name_override =
        argument.substr(std::string("--project-name=").size());
    return true;
  }

  if (argument.rfind("--module-filter=", 0) == 0) {
    options.module_filter =
        argument.substr(std::string("--module-filter=").size());
    return true;
  }

  if (argument.rfind("--threads=", 0) == 0) {
    const auto threads =
        ParseThreadCount(argument.substr(std::string("--threads=").size()));
    if (!threads.has_value()) {
      error = build_parse_error("invalid thread count", "option", argument);
      return false;
    }
    options.threads = *threads;
    return true;
  }

  if (argument == "--verbose") {
    options.verbose = true;
    return true;
  }

  error = build_parse_error("unsupported option", "option", argument);
  return false;
}

bool validate_required_options(const CliOptions &options,
                               const bool module_option_seen,
                               CliParseError &error) {
  if (options.metrics.empty()) {
    error = build_parse_error("at least one metric is required");
    return false;
  }

  if (!module_option_seen) {
    error = build_parse_error("missing required option", "option",
                              "--module=<kind>");
    return false;
  }

  return true;
}

std::optional<CliOptions> ParseCli(const std::vector<std::string> &args,
                                   CliParseError &error) {
  if (args.empty() || (args.size() == 1U && args.front() == "--help")) {
    std::cout << archscope::core::HelpText();
    return std::nullopt;
  }

  if (args.size() == 1U && args.front() == "--version") {
    std::cout << archscope::core::version_string() << '\n';
    return std::nullopt;
  }

  if (args.size() < 3U) {
    error = build_parse_error("expected a compile_commands.json path, at least "
                              "one metric, and --module=<kind>");
    return std::nullopt;
  }

  CliOptions options;
  std::size_t index = 1U;
  if (!parse_positional_arguments(args, options, index, error)) {
    return std::nullopt;
  }

  bool module_option_seen = false;
  for (; index < args.size(); ++index) {
    if (!parse_option_argument(args[index], options, module_option_seen,
                               error)) {
      return std::nullopt;
    }
  }

  if (!validate_required_options(options, module_option_seen, error)) {
    return std::nullopt;
  }

  return options;
}

std::string DeriveProjectName(const CliOptions &options) {
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

std::string ModuleKindName(const archscope::core::ModuleKind module_kind) {
  switch (module_kind) {
  case archscope::core::ModuleKind::namespace_module:
    return "namespace";
  case archscope::core::ModuleKind::translation_unit:
    return "translation_unit";
  case archscope::core::ModuleKind::header:
    return "header";
  case archscope::core::ModuleKind::compilation_target:
    return "compilation_target";
  }

  return "unknown";
}

std::string CountLabel(const std::size_t count, const char *singular,
                       const char *plural) {
  std::ostringstream stream;
  const char *label = plural;
  if (count == 1U) {
    label = singular;
  }
  stream << count << ' ' << label;
  return stream.str();
}

void WriteInfoLog(const bool verbose, const std::string &message) {
  if (!verbose) {
    return;
  }
  std::cerr << archscope::core::FormatInfoText(message);
}

enum class CliExitCode : int {
  success = 0,
  usage_error = 2,
  compilation_database_error = 3,
  analysis_error = 4,
  internal_error = 5,
};

enum class RunErrorType {
  usage,
  compilation_database,
  analysis,
  internal,
};

struct RunError {
  RunErrorType type;
  std::string title;
  std::string message;
  std::vector<archscope::core::CliDetail> details;
};

int map_error_to_exit_code(const RunErrorType type) {
  switch (type) {
  case RunErrorType::usage:
    return static_cast<int>(CliExitCode::usage_error);
  case RunErrorType::compilation_database:
    return static_cast<int>(CliExitCode::compilation_database_error);
  case RunErrorType::analysis:
    return static_cast<int>(CliExitCode::analysis_error);
  case RunErrorType::internal:
    return static_cast<int>(CliExitCode::internal_error);
  }

  return static_cast<int>(CliExitCode::internal_error);
}

struct LoadDatabaseStepResult {
  archscope::core::CompilationDatabase database;
};

std::optional<RunError> load_database_step(const CliOptions &options,
                                           LoadDatabaseStepResult &result) {
  WriteInfoLog(options.verbose, "loading compilation database from " +
                                    options.compile_commands_path.string());
  const auto database =
      archscope::core::load_compilation_database(options.compile_commands_path);
  if (!database.has_value()) {
    return RunError{RunErrorType::compilation_database,
                    "compilation database error",
                    database.error().message,
                    {{"path", database.error().context}}};
  }

  result.database = database.value();
  WriteInfoLog(options.verbose,
               "loaded " + CountLabel(result.database.entries.size(),
                                      "compilation database entry",
                                      "compilation database entries"));
  return std::nullopt;
}

struct ExtractAnalysisStepResult {
  archscope::clang_backend::ExtractionResult extraction;
};

std::optional<RunError>
extract_analysis_step(const CliOptions &options,
                      const LoadDatabaseStepResult &database_result,
                      ExtractAnalysisStepResult &result) {
  WriteInfoLog(options.verbose,
               "analyzing " +
                   CountLabel(database_result.database.entries.size(),
                              "translation unit", "translation units") +
                   " with " + std::to_string(options.threads) + " thread(s)");
  const auto extracted_analysis = archscope::clang_backend::extract_analysis(
      database_result.database, options.threads);
  if (!extracted_analysis.has_value()) {
    std::vector<archscope::core::CliDetail> details;
    for (const std::string &failed_translation_unit :
         extracted_analysis.error().failed_translation_units) {
      details.emplace_back("translation unit", failed_translation_unit);
    }
    return RunError{RunErrorType::analysis, "analysis error",
                    extracted_analysis.error().message, std::move(details)};
  }

  result.extraction = extracted_analysis.value();
  return std::nullopt;
}

struct ProjectAndComputeReportStepResult {
  std::string markdown;
};

ProjectAndComputeReportStepResult
project_and_compute_report_step(const CliOptions &options,
                                const ExtractAnalysisStepResult &analysis) {
  const auto analysis_result = archscope::clang_backend::project_analysis(
      analysis.extraction, options.module_kind);
  WriteInfoLog(
      options.verbose,
      "projected analysis into " +
          CountLabel(analysis_result.modules.size(), "module", "modules") +
          " using " + ModuleKindName(options.module_kind) + " ownership");
  const auto metric_registry = archscope::core::MetricRegistry::with_defaults();

  archscope::core::ReportModel report{
      DeriveProjectName(options),
      {},
  };

  for (const archscope::core::ModuleId &module_id : analysis_result.modules) {
    if (!archscope::core::matches_module_filter(
            options.module_kind, module_id.value, options.module_filter)) {
      continue;
    }
    archscope::core::ReportModule module{
        module_id.value,
        metric_registry.compute(analysis_result, module_id, options.metrics),
    };
    report.modules.push_back(module);
  }

  return ProjectAndComputeReportStepResult{
      archscope::core::to_markdown(report)};
}

std::optional<RunError>
write_report_step(const CliOptions &options,
                  const ProjectAndComputeReportStepResult &report_result) {
  WriteInfoLog(options.verbose,
               "writing Markdown report to " + options.report_path.string());
  std::string error_message;
  if (!archscope::core::write_report_file(
          options.report_path, report_result.markdown, error_message)) {
    return RunError{RunErrorType::internal,
                    "internal error",
                    error_message,
                    {{"report", options.report_path.string()}}};
  }
  WriteInfoLog(options.verbose, "report written successfully");

  return std::nullopt;
}

int RunCli(const std::vector<std::string> &args) {
  CliParseError parse_error;
  const auto parsed = ParseCli(args, parse_error);
  if (!parsed.has_value()) {
    if (parse_error.message.empty()) {
      return static_cast<int>(CliExitCode::success);
    }

    std::cerr << archscope::core::FormatErrorText(
                     "usage error", parse_error.message, parse_error.details)
              << '\n'
              << archscope::core::HelpText();
    return map_error_to_exit_code(RunErrorType::usage);
  }

  LoadDatabaseStepResult database_result;
  if (const auto error = load_database_step(*parsed, database_result);
      error.has_value()) {
    std::cerr << archscope::core::FormatErrorText(error->title, error->message,
                                                  error->details);
    return map_error_to_exit_code(error->type);
  }

  ExtractAnalysisStepResult analysis_result;
  if (const auto error =
          extract_analysis_step(*parsed, database_result, analysis_result);
      error.has_value()) {
    std::cerr << archscope::core::FormatErrorText(error->title, error->message,
                                                  error->details);
    return map_error_to_exit_code(error->type);
  }

  const auto report_result =
      project_and_compute_report_step(*parsed, analysis_result);
  if (const auto error = write_report_step(*parsed, report_result);
      error.has_value()) {
    std::cerr << archscope::core::FormatErrorText(error->title, error->message,
                                                  error->details);
    return map_error_to_exit_code(error->type);
  }

  return static_cast<int>(CliExitCode::success);
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::vector<std::string> args;
    std::size_t argument_count = 0U;
    if (argc > 0) {
      argument_count = static_cast<std::size_t>(argc - 1);
    }
    args.reserve(argument_count);
    for (int index = 1; index < argc; ++index) {
      args.emplace_back(argv[index]);
    }
    return RunCli(args);
  } catch (const std::exception &error) {
    std::cerr << archscope::core::FormatErrorText("internal error",
                                                  error.what());
    return static_cast<int>(CliExitCode::internal_error);
  } catch (...) {
    std::cerr << archscope::core::FormatErrorText("internal error",
                                                  "unexpected exception");
    return static_cast<int>(CliExitCode::internal_error);
  }
}
