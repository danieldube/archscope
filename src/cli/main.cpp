#include "core/cli_text.hpp"
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
#include <vector>

namespace {

unsigned DefaultThreadCount() {
  const unsigned detected = std::thread::hardware_concurrency();
  return detected == 0U ? 1U : detected;
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

bool IsOption(const std::string &argument) {
  return argument.rfind("--", 0) == 0;
}

std::optional<archscope::core::MetricId>
ParseMetricId(const std::string &value) {
  if (value == "abstractness") {
    return archscope::core::MetricId::abstractness;
  }
  if (value == "instability") {
    return archscope::core::MetricId::instability;
  }
  if (value == "abstract_type_count") {
    return archscope::core::MetricId::abstract_type_count;
  }
  if (value == "concrete_type_count") {
    return archscope::core::MetricId::concrete_type_count;
  }
  if (value == "type_count") {
    return archscope::core::MetricId::type_count;
  }
  if (value == "distance_from_main_sequence") {
    return archscope::core::MetricId::distance_from_main_sequence;
  }
  return std::nullopt;
}

std::optional<archscope::core::ModuleKind>
ParseModuleKind(const std::string &value) {
  if (value == "translation_unit") {
    return archscope::core::ModuleKind::translation_unit;
  }
  if (value == "namespace") {
    return archscope::core::ModuleKind::namespace_module;
  }
  if (value == "header") {
    return archscope::core::ModuleKind::header;
  }
  return std::nullopt;
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
    error.message = "expected a compile_commands.json path, at least one "
                    "metric, and --module=<kind>";
    return std::nullopt;
  }

  CliOptions options;
  options.compile_commands_path = args.front();

  std::size_t index = 1U;
  for (; index < args.size(); ++index) {
    const std::string &argument = args[index];
    if (IsOption(argument)) {
      break;
    }

    const auto metric = ParseMetricId(argument);
    if (!metric.has_value()) {
      error.message = "unsupported metric";
      error.details = {{"metric", argument}};
      return std::nullopt;
    }
    options.metrics.push_back(*metric);
  }

  if (options.metrics.empty()) {
    error.message = "at least one metric is required";
    return std::nullopt;
  }

  for (; index < args.size(); ++index) {
    const std::string &argument = args[index];
    if (argument.rfind("--module=", 0) == 0) {
      const auto module_kind =
          ParseModuleKind(argument.substr(std::string("--module=").size()));
      if (!module_kind.has_value()) {
        error.message = "unsupported module kind";
        error.details = {{"option", argument}};
        return std::nullopt;
      }
      options.module_kind = *module_kind;
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
    if (argument.rfind("--module-filter=", 0) == 0) {
      options.module_filter =
          argument.substr(std::string("--module-filter=").size());
      continue;
    }
    if (argument.rfind("--threads=", 0) == 0) {
      const auto threads =
          ParseThreadCount(argument.substr(std::string("--threads=").size()));
      if (!threads.has_value()) {
        error.message = "invalid thread count";
        error.details = {{"option", argument}};
        return std::nullopt;
      }
      options.threads = *threads;
      continue;
    }
    if (argument == "--verbose") {
      options.verbose = true;
      continue;
    }

    error.message = "unsupported option";
    error.details = {{"option", argument}};
    return std::nullopt;
  }

  if (std::none_of(args.begin(), args.end(), [](const std::string &argument) {
        return argument.rfind("--module=", 0) == 0;
      })) {
    error.message = "missing required option";
    error.details = {{"option", "--module=<kind>"}};
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
  }

  return "unknown";
}

std::string CountLabel(const std::size_t count, const char *singular,
                       const char *plural) {
  std::ostringstream stream;
  stream << count << ' ' << (count == 1U ? singular : plural);
  return stream.str();
}

void WriteInfoLog(const bool verbose, const std::string &message) {
  if (!verbose) {
    return;
  }
  std::cerr << archscope::core::FormatInfoText(message);
}

int RunCli(const std::vector<std::string> &args) {
  CliParseError parse_error;
  const auto parsed = ParseCli(args, parse_error);
  if (!parsed.has_value()) {
    if (parse_error.message.empty()) {
      return 0;
    }

    std::cerr << archscope::core::FormatErrorText(
                     "usage error", parse_error.message, parse_error.details)
              << '\n'
              << archscope::core::HelpText();
    return 2;
  }

  WriteInfoLog(parsed->verbose, "loading compilation database from " +
                                    parsed->compile_commands_path.string());
  const auto database =
      archscope::core::load_compilation_database(parsed->compile_commands_path);
  if (!database.has_value()) {
    std::cerr << archscope::core::FormatErrorText(
        "compilation database error", database.error().message,
        {{"path", database.error().context}});
    return 3;
  }

  WriteInfoLog(parsed->verbose,
               "loaded " + CountLabel(database.value().entries.size(),
                                      "compilation database entry",
                                      "compilation database entries"));
  WriteInfoLog(parsed->verbose,
               "analyzing " +
                   CountLabel(database.value().entries.size(),
                              "translation unit", "translation units") +
                   " with " + std::to_string(parsed->threads) + " thread(s)");
  const auto extracted_analysis = archscope::clang_backend::extract_analysis(
      database.value(), parsed->threads);
  if (!extracted_analysis.has_value()) {
    std::vector<archscope::core::CliDetail> details;
    for (const std::string &failed_translation_unit :
         extracted_analysis.error().failed_translation_units) {
      details.emplace_back("translation unit", failed_translation_unit);
    }
    std::cerr << archscope::core::FormatErrorText(
        "analysis error", extracted_analysis.error().message, details);
    return 4;
  }

  const auto analysis_result = archscope::clang_backend::project_analysis(
      extracted_analysis.value(), parsed->module_kind);
  WriteInfoLog(
      parsed->verbose,
      "projected analysis into " +
          CountLabel(analysis_result.modules.size(), "module", "modules") +
          " using " + ModuleKindName(parsed->module_kind) + " ownership");
  const auto metric_registry = archscope::core::MetricRegistry::with_defaults();

  archscope::core::ReportModel report{
      DeriveProjectName(*parsed),
      {},
  };

  for (const archscope::core::ModuleId &module_id : analysis_result.modules) {
    if (!archscope::core::matches_module_filter(
            parsed->module_kind, module_id.value, parsed->module_filter)) {
      continue;
    }
    archscope::core::ReportModule module{
        module_id.value,
        metric_registry.compute(analysis_result, module_id, parsed->metrics),
    };
    report.modules.push_back(module);
  }

  const std::string markdown = archscope::core::to_markdown(report);
  WriteInfoLog(parsed->verbose,
               "writing Markdown report to " + parsed->report_path.string());
  std::string error_message;
  if (!archscope::core::write_report_file(parsed->report_path, markdown,
                                          error_message)) {
    std::cerr << archscope::core::FormatErrorText(
        "internal error", error_message,
        {{"report", parsed->report_path.string()}});
    return 5;
  }
  WriteInfoLog(parsed->verbose, "report written successfully");

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
    return RunCli(args);
  } catch (const std::exception &error) {
    std::cerr << archscope::core::FormatErrorText("internal error",
                                                  error.what());
    return 5;
  } catch (...) {
    std::cerr << archscope::core::FormatErrorText("internal error",
                                                  "unexpected exception");
    return 5;
  }
}
