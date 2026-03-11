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
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

unsigned default_thread_count() {
  const unsigned detected = std::thread::hardware_concurrency();
  return detected == 0U ? 1U : detected;
}

unsigned clamp_thread_count(const unsigned requested) {
  const unsigned maximum = default_thread_count();
  if (requested == 0U) {
    return 1U;
  }
  return std::min(requested, maximum);
}

std::optional<unsigned> parse_thread_count(const std::string &value) {
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

  return clamp_thread_count(static_cast<unsigned>(requested));
}

struct CliOptions {
  std::filesystem::path compile_commands_path;
  std::vector<archscope::core::MetricId> metrics;
  archscope::core::ModuleKind module_kind =
      archscope::core::ModuleKind::translation_unit;
  std::optional<std::string> module_filter;
  std::filesystem::path report_path = "architecture-metrics.md";
  std::optional<std::string> project_name_override;
  unsigned threads = default_thread_count();
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

std::optional<archscope::core::ModuleKind>
parse_module_kind(const std::string &value) {
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
      const auto module_kind =
          parse_module_kind(argument.substr(std::string("--module=").size()));
      if (!module_kind.has_value()) {
        error_message = "unsupported module kind";
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
          parse_thread_count(argument.substr(std::string("--threads=").size()));
      if (!threads.has_value()) {
        error_message = "invalid thread count";
        return std::nullopt;
      }
      options.threads = *threads;
      continue;
    }

    error_message = "unsupported option: " + argument;
    return std::nullopt;
  }

  if (std::none_of(args.begin(), args.end(), [](const std::string &argument) {
        return argument.rfind("--module=", 0) == 0;
      })) {
    error_message = "missing required option: --module=<kind>";
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

  const auto extracted_analysis = archscope::clang_backend::extract_analysis(
      database.value(), parsed->threads);
  if (!extracted_analysis.has_value()) {
    std::cerr << "error: " << extracted_analysis.error().message;
    if (!extracted_analysis.error().failed_translation_units.empty()) {
      std::cerr << ": "
                << extracted_analysis.error().failed_translation_units.front();
    }
    std::cerr << '\n';
    return 4;
  }

  const auto analysis_result = archscope::clang_backend::project_analysis(
      extracted_analysis.value(), parsed->module_kind);
  const auto metric_registry = archscope::core::MetricRegistry::with_defaults();

  archscope::core::ReportModel report{
      derive_project_name(*parsed),
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
