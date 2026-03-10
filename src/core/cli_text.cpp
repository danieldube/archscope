#include "core/cli_text.hpp"

#include "core/version.hpp"

namespace archscope::core {

std::string help_text() {
  return "Usage: archscope <compile_commands.json> <metrics...> "
         "--module=<kind> [--report=<path>] [--project-name=<name>]\n"
         "       archscope [--help] [--version]\n"
         "\n"
         "Supported metric ids: abstractness, instability, "
         "distance_from_main_sequence\n"
         "\n"
         "Options:\n"
         "  --module=<kind>         Module kind (supported: namespace, "
         "translation_unit, header).\n"
         "  --module-filter=<text>  Filter output modules. Namespace "
         "filters use prefix matching; translation-unit and header "
         "filters use substring matching.\n"
         "  --report=<path>         Output Markdown report path.\n"
         "  --project-name=<name>   Override the project name shown in the "
         "report.\n"
         "  --help                  Show this help message and exit.\n"
         "  --version               Show the executable version and exit.\n";
}

} // namespace archscope::core
