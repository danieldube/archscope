#include "core/cli_text.hpp"

#include "core/cli_tokens.hpp"
#include "core/version.hpp"

namespace archscope::core {

std::string HelpText() {
  std::string help =
      "Usage: archscope <compile_commands.json> <metrics...> "
      "--module=<kind> [--report=<path>] [--project-name=<name>] "
      "[--threads=<n>] [--verbose]\n"
      "       archscope [--help] [--version]\n"
      "\n";
  help += "Supported metric ids: " + supported_metrics_text() + "\n";
  help += "\n"
          "Options:\n";
  help += "  --module=<kind>         Module kind (supported: " +
          supported_module_kinds_text() + ").\n";
  help +=
      "  --module-filter=<text>  Filter output modules. Namespace "
      "filters use prefix matching; translation-unit and header "
      "filters use substring matching; compilation-target filters "
      "use exact matching.\n"
      "  --report=<path>         Output Markdown report path.\n"
      "  --project-name=<name>   Override the project name shown in the "
      "report.\n"
      "  --threads=<n>           Requested analysis parallelism for "
      "per-translation-unit extraction.\n"
      "  --verbose               Emit progress logs and extended error "
      "context to stderr.\n"
      "  --help                  Show this help message and exit.\n"
      "  --version               Show the executable version and exit.\n";
  return help;
}

std::string FormatErrorText(const std::string &category,
                            const std::string &message,
                            const std::vector<CliDetail> &details) {
  std::string formatted = "error: " + category + "\n";
  formatted += "  message: " + message + "\n";
  for (const auto &[label, value] : details) {
    if (value.empty()) {
      continue;
    }
    formatted += "  ";
    formatted += label;
    formatted += ": ";
    formatted += value;
    formatted += '\n';
  }
  return formatted;
}

std::string FormatInfoText(const std::string &message) {
  return "info: " + message + "\n";
}

} // namespace archscope::core
