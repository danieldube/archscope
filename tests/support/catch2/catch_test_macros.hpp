#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Catch {

struct TestCase {
    const char *name;
    void (*function)();
};

class AssertionFailure : public std::runtime_error {
  public:
    AssertionFailure(std::string expression, const char *file_name,
                     int line_number);
};

class Registry {
  public:
    static void add(const char *name, void (*function)());
    static const std::vector<TestCase> &all();
};

class TestRegistrar {
  public:
    TestRegistrar(const char *name, void (*function)());
};

int run_all_tests();

} // namespace Catch

#define CATCH_JOIN_IMPL(left, right) left##right
#define CATCH_JOIN(left, right) CATCH_JOIN_IMPL(left, right)
#define CATCH_UNIQ(base) CATCH_JOIN(base, __LINE__)

// clang-format off
#define TEST_CASE(name) \
    static void CATCH_UNIQ(test_fn_)(); \
    static ::Catch::TestRegistrar CATCH_UNIQ(test_reg_)( \
        name, &CATCH_UNIQ(test_fn_)); \
    static void CATCH_UNIQ(test_fn_)()

#define REQUIRE(expression) \
    do { \
        if (!(expression)) { \
            throw ::Catch::AssertionFailure( \
                #expression, __FILE__, __LINE__); \
        } \
    } while (false)
// clang-format on
