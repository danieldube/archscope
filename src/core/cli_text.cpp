#include "core/cli_text.hpp"

#include "core/version.hpp"

namespace archscope::core {

std::string help_text() {
    return "Usage: archscope [--help] [--version]\n"
           "\n"
           "ArchScope bootstrap CLI. Full analysis commands arrive in later "
           "increments.\n"
           "\n"
           "Options:\n"
           "  --help       Show this help message and exit.\n"
           "  --version    Show the executable version and exit.\n";
}

} // namespace archscope::core
