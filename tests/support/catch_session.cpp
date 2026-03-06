#include "catch2/catch_test_macros.hpp"

#include <exception>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

namespace Catch {

namespace {

std::vector<TestCase> &mutable_registry() {
    static std::vector<TestCase> tests;
    return tests;
}

} // namespace

AssertionFailure::AssertionFailure(std::string expression,
                                   const char *file_name, int line_number)
    : std::runtime_error([&expression, file_name, line_number]() {
          std::ostringstream builder;
          builder << file_name << ':' << line_number << " REQUIRE("
                  << expression << ") failed";
          return builder.str();
      }()) {}

void Registry::add(const char *name, void (*function)()) {
    mutable_registry().push_back(TestCase{name, function});
}

const std::vector<TestCase> &Registry::all() { return mutable_registry(); }

TestRegistrar::TestRegistrar(const char *name, void (*function)()) {
    Registry::add(name, function);
}

int run_all_tests() {
    int failures = 0;

    for (const TestCase &test_case : Registry::all()) {
        try {
            test_case.function();
        } catch (const std::exception &error) {
            std::cerr << "[FAILED] " << test_case.name << '\n'
                      << error.what() << '\n';
            ++failures;
        } catch (...) {
            std::cerr << "[FAILED] " << test_case.name << '\n'
                      << "unknown exception\n";
            ++failures;
        }
    }

    if (failures == 0) {
        std::cout << "All tests passed (" << Registry::all().size() << ")\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed\n";
    return 1;
}

} // namespace Catch
