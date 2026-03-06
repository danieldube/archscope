#include "core/cli_text.hpp"
#include "core/version.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

int run_cli(const std::vector<std::string> &args) {
    if (args.empty() || (args.size() == 1U && args.front() == "--help")) {
        std::cout << archscope::core::help_text();
        return 0;
    }

    if (args.size() == 1U && args.front() == "--version") {
        std::cout << archscope::core::version_string() << '\n';
        return 0;
    }

    std::cerr << "error: unsupported arguments for bootstrap increment\n\n"
              << archscope::core::help_text();
    return 2;
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
