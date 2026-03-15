cmake_minimum_required(VERSION 3.26)

project(
  ArchScope
  VERSION 0.1.0
  LANGUAGES C CXX)

include(FetchContent)
include(cmake/CompilerWarnings.cmake)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ARCHSCOPE_LLVM_VERSION
    "18"
    CACHE STRING "Preferred LLVM/Clang major version for LibTooling")

option(ARCHSCOPE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

set(ARCHSCOPE_LLVM_HINT
    "/usr/lib/llvm-${ARCHSCOPE_LLVM_VERSION}/lib/cmake/clang")
set(ARCHSCOPE_LLVM_CONFIG_HINT
    "/usr/lib/llvm-${ARCHSCOPE_LLVM_VERSION}/lib/cmake/llvm")
find_package(LLVM CONFIG REQUIRED HINTS "${ARCHSCOPE_LLVM_CONFIG_HINT}")
find_package(Clang CONFIG REQUIRED HINTS "${ARCHSCOPE_LLVM_HINT}")

if(BUILD_TESTING)
  set(ARCHSCOPE_LOCAL_CATCH2_DIRS
      "${PROJECT_BINARY_DIR}/_deps/catch2-src"
      "${PROJECT_SOURCE_DIR}/cmake-build-debug/_deps/catch2-src")
  foreach(archscope_catch2_dir IN LISTS ARCHSCOPE_LOCAL_CATCH2_DIRS)
    if(EXISTS "${archscope_catch2_dir}/CMakeLists.txt")
      set(FETCHCONTENT_SOURCE_DIR_CATCH2 "${archscope_catch2_dir}")
      break()
    endif()
  endforeach()

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
  include(Catch)
endif()

add_library(
  archscope_core STATIC
  src/core/analysis.cpp
  src/core/cli_text.cpp
  src/core/cli_tokens.cpp
  src/core/compilation_database.cpp
  src/core/compilation_database_loader.cpp
  src/core/compile_commands_decoder.cpp
  src/core/json_value.cpp
  src/core/metrics.cpp
  src/core/module_filter.cpp
  src/core/report.cpp
  src/core/version.cpp)
add_library(ArchScope::core ALIAS archscope_core)

add_library(
  archscope_clang STATIC
  src/clang/analysis_projection.cpp src/clang/namespace_module_resolver.cpp
  src/clang/parallel_executor.cpp src/clang/tool_runner.cpp)
add_library(ArchScope::clang ALIAS archscope_clang)

target_include_directories(archscope_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_features(archscope_core PUBLIC cxx_std_17)
archscope_set_common_warnings(archscope_core)

target_include_directories(archscope_clang PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(archscope_clang SYSTEM
                           PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(archscope_clang PUBLIC ArchScope::core clangTooling)
target_compile_features(archscope_clang PUBLIC cxx_std_17)
archscope_set_common_warnings(archscope_clang)

add_executable(archscope src/cli/main.cpp)
target_link_libraries(archscope PRIVATE ArchScope::core ArchScope::clang)
target_compile_features(archscope PRIVATE cxx_std_17)
archscope_set_common_warnings(archscope)

if(BUILD_TESTING)
  add_executable(
    archscope_tests
    tests/unit/abstractness_metric_tests.cpp
    tests/unit/analysis_result_tests.cpp
    tests/unit/analysis_projection_tests.cpp
    tests/unit/cli_text_tests.cpp
    tests/unit/cli_tokens_tests.cpp
    tests/unit/clang_tool_runner_tests.cpp
    tests/unit/compile_commands_decoder_tests.cpp
    tests/unit/compilation_database_loader_tests.cpp
    tests/unit/dependency_graph_builder_tests.cpp
    tests/unit/distance_metric_tests.cpp
    tests/unit/instability_metric_tests.cpp
    tests/unit/json_value_tests.cpp
    tests/unit/metric_registry_tests.cpp
    tests/unit/module_filter_tests.cpp
    tests/unit/namespace_module_resolver_tests.cpp
    tests/unit/parallel_executor_tests.cpp
    tests/unit/report_writer_tests.cpp
    tests/unit/type_count_metric_tests.cpp
    tests/unit/version_tests.cpp)
  target_include_directories(archscope_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_include_directories(
    archscope_tests SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
  target_link_libraries(
    archscope_tests PRIVATE ArchScope::core ArchScope::clang
                            Catch2::Catch2WithMain)
  add_dependencies(archscope_tests archscope)
  target_compile_features(archscope_tests PRIVATE cxx_std_17)
  archscope_set_common_warnings(archscope_tests)

  set(ARCHSCOPE_EXPECTED_FILE
      "${PROJECT_SOURCE_DIR}/tests/system/expected/version.txt")
  set(ARCHSCOPE_SYSTEM_EXPECTED_DIR
      "${PROJECT_SOURCE_DIR}/tests/system/expected")
  set(ARCHSCOPE_SYSTEM_SCRIPT_DIR "${PROJECT_SOURCE_DIR}/tests/system")
  set(ARCHSCOPE_SYSTEM_REPORT_DIR "${PROJECT_BINARY_DIR}/tests/system")
  set(ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_CHECK_SCRIPT_NAME
      "check_translation_unit_distance_report.cmake")
  set(ARCHSCOPE_NAMESPACE_FILTER_CHECK_SCRIPT_NAME
      "check_namespace_filter_report.cmake")
  set(ARCHSCOPE_HEADER_FILTER_CHECK_SCRIPT_NAME
      "check_header_filter_report.cmake")
  set(ARCHSCOPE_COMPILATION_TARGET_CHECK_SCRIPT_NAME
      "check_compilation_target_report.cmake")
  set(ARCHSCOPE_PARALLEL_DETERMINISM_CHECK_SCRIPT_NAME
      "check_parallel_determinism.cmake")
  set(ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT_NAME "check_error_case.cmake")
  set(ARCHSCOPE_VERBOSE_LOGGING_CHECK_SCRIPT_NAME
      "check_verbose_logging.cmake")
  set(ARCHSCOPE_INTERNAL_ERROR_CHECK_SCRIPT_NAME
      "check_internal_error_case.cmake")
  set(ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_REPORT_NAME
      "translation-unit-distance-report.md")
  set(ARCHSCOPE_COMPILATION_TARGET_REPORT_NAME "compilation-target-report.md")
  set(ARCHSCOPE_NAMESPACE_FILTER_REPORT_NAME "namespace-filter-report.md")
  set(ARCHSCOPE_HEADER_FILTER_REPORT_NAME "header-filter-report.md")
  set(ARCHSCOPE_PARALLEL_REPORT_DIR_NAME "parallel-determinism")
  set(ARCHSCOPE_VERSION_CHECK_SCRIPT
      "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}/check_version.cmake")
  set(ARCHSCOPE_PLACEHOLDER_CHECK_SCRIPT
      "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}/check_placeholder_report.cmake")
  set(ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_CHECK_SCRIPT
      "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_CHECK_SCRIPT
             "${ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_NAMESPACE_FILTER_CHECK_SCRIPT "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_NAMESPACE_FILTER_CHECK_SCRIPT
             "${ARCHSCOPE_NAMESPACE_FILTER_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_HEADER_FILTER_CHECK_SCRIPT "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_HEADER_FILTER_CHECK_SCRIPT
             "${ARCHSCOPE_HEADER_FILTER_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_COMPILATION_TARGET_CHECK_SCRIPT
      "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_COMPILATION_TARGET_CHECK_SCRIPT
             "${ARCHSCOPE_COMPILATION_TARGET_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_PARALLEL_DETERMINISM_CHECK_SCRIPT
      "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_PARALLEL_DETERMINISM_CHECK_SCRIPT
             "${ARCHSCOPE_PARALLEL_DETERMINISM_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT
             "${ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_VERBOSE_LOGGING_CHECK_SCRIPT "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_VERBOSE_LOGGING_CHECK_SCRIPT
             "${ARCHSCOPE_VERBOSE_LOGGING_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_INTERNAL_ERROR_CHECK_SCRIPT "${ARCHSCOPE_SYSTEM_SCRIPT_DIR}")
  cmake_path(APPEND ARCHSCOPE_INTERNAL_ERROR_CHECK_SCRIPT
             "${ARCHSCOPE_INTERNAL_ERROR_CHECK_SCRIPT_NAME}")
  set(ARCHSCOPE_PLACEHOLDER_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/placeholder_project")
  set(ARCHSCOPE_PLACEHOLDER_FIXTURE
      "${ARCHSCOPE_PLACEHOLDER_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_PLACEHOLDER_EXPECTED
      "${ARCHSCOPE_SYSTEM_EXPECTED_DIR}/placeholder-report.md")
  set(ARCHSCOPE_PLACEHOLDER_REPORT
      "${ARCHSCOPE_SYSTEM_REPORT_DIR}/placeholder-report.md")
  set(ARCHSCOPE_INVALID_REPORT_OUTPUT
      "${ARCHSCOPE_PLACEHOLDER_FIXTURE}/report.md")
  set(ARCHSCOPE_DEPENDENCY_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/dependency_project")
  set(ARCHSCOPE_DEPENDENCY_FIXTURE
      "${ARCHSCOPE_DEPENDENCY_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_DEPENDENCY_EXPECTED "${ARCHSCOPE_SYSTEM_EXPECTED_DIR}")
  cmake_path(APPEND ARCHSCOPE_DEPENDENCY_EXPECTED
             "${ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_REPORT_NAME}")
  set(ARCHSCOPE_DEPENDENCY_REPORT "${ARCHSCOPE_SYSTEM_REPORT_DIR}")
  cmake_path(APPEND ARCHSCOPE_DEPENDENCY_REPORT
             "${ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_REPORT_NAME}")
  set(ARCHSCOPE_COMPILATION_TARGET_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/compilation_target_project")
  set(ARCHSCOPE_COMPILATION_TARGET_FIXTURE
      "${ARCHSCOPE_COMPILATION_TARGET_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_COMPILATION_TARGET_EXPECTED "${ARCHSCOPE_SYSTEM_EXPECTED_DIR}")
  cmake_path(APPEND ARCHSCOPE_COMPILATION_TARGET_EXPECTED
             "${ARCHSCOPE_COMPILATION_TARGET_REPORT_NAME}")
  set(ARCHSCOPE_COMPILATION_TARGET_REPORT "${ARCHSCOPE_SYSTEM_REPORT_DIR}")
  cmake_path(APPEND ARCHSCOPE_COMPILATION_TARGET_REPORT
             "${ARCHSCOPE_COMPILATION_TARGET_REPORT_NAME}")
  set(ARCHSCOPE_PARALLEL_REPORT_DIR "${ARCHSCOPE_SYSTEM_REPORT_DIR}")
  cmake_path(APPEND ARCHSCOPE_PARALLEL_REPORT_DIR
             "${ARCHSCOPE_PARALLEL_REPORT_DIR_NAME}")
  set(ARCHSCOPE_PARALLEL_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/parallel_project")
  set(ARCHSCOPE_PARALLEL_FIXTURE
      "${ARCHSCOPE_PARALLEL_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_PARSE_FAILURE_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/parse_failure_project")
  set(ARCHSCOPE_PARSE_FAILURE_FIXTURE
      "${ARCHSCOPE_PARSE_FAILURE_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_MISSING_COMPILE_DB
      "${PROJECT_BINARY_DIR}/tests/system/missing-compile_commands.json")
  set(ARCHSCOPE_NAMESPACE_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/namespace_project")
  set(ARCHSCOPE_NAMESPACE_FIXTURE
      "${ARCHSCOPE_NAMESPACE_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_NAMESPACE_EXPECTED "${ARCHSCOPE_SYSTEM_EXPECTED_DIR}")
  cmake_path(APPEND ARCHSCOPE_NAMESPACE_EXPECTED
             "${ARCHSCOPE_NAMESPACE_FILTER_REPORT_NAME}")
  set(ARCHSCOPE_NAMESPACE_REPORT "${ARCHSCOPE_SYSTEM_REPORT_DIR}")
  cmake_path(APPEND ARCHSCOPE_NAMESPACE_REPORT
             "${ARCHSCOPE_NAMESPACE_FILTER_REPORT_NAME}")
  set(ARCHSCOPE_HEADER_FIXTURE_DIR
      "${PROJECT_SOURCE_DIR}/tests/fixtures/header_project")
  set(ARCHSCOPE_HEADER_FIXTURE
      "${ARCHSCOPE_HEADER_FIXTURE_DIR}/compile_commands.json")
  set(ARCHSCOPE_HEADER_EXPECTED "${ARCHSCOPE_SYSTEM_EXPECTED_DIR}")
  cmake_path(APPEND ARCHSCOPE_HEADER_EXPECTED
             "${ARCHSCOPE_HEADER_FILTER_REPORT_NAME}")
  set(ARCHSCOPE_HEADER_REPORT "${ARCHSCOPE_SYSTEM_REPORT_DIR}")
  cmake_path(APPEND ARCHSCOPE_HEADER_REPORT
             "${ARCHSCOPE_HEADER_FILTER_REPORT_NAME}")
  catch_discover_tests(archscope_tests)
  add_test(
    NAME archscope_system_version
    COMMAND
      ${CMAKE_COMMAND} -DARCHSCOPE_BIN=$<TARGET_FILE:archscope>
      -DEXPECTED_FILE=${ARCHSCOPE_EXPECTED_FILE} -P
      ${ARCHSCOPE_VERSION_CHECK_SCRIPT})
  add_test(
    NAME archscope_system_placeholder_report
    COMMAND
      ${CMAKE_COMMAND} -DARCHSCOPE_BIN=$<TARGET_FILE:archscope>
      -DFIXTURE_DB=${ARCHSCOPE_PLACEHOLDER_FIXTURE}
      -DEXPECTED_FILE=${ARCHSCOPE_PLACEHOLDER_EXPECTED}
      -DREPORT_OUTPUT=${ARCHSCOPE_PLACEHOLDER_REPORT} -P
      ${ARCHSCOPE_PLACEHOLDER_CHECK_SCRIPT})
  add_test(
    NAME archscope_system_translation_unit_distance
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_DEPENDENCY_FIXTURE}"
      "-DEXPECTED_FILE=${ARCHSCOPE_DEPENDENCY_EXPECTED}"
      "-DREPORT_OUTPUT=${ARCHSCOPE_DEPENDENCY_REPORT}" -P
      "${ARCHSCOPE_TRANSLATION_UNIT_DISTANCE_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_namespace_filter
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_NAMESPACE_FIXTURE}"
      "-DEXPECTED_FILE=${ARCHSCOPE_NAMESPACE_EXPECTED}"
      "-DREPORT_OUTPUT=${ARCHSCOPE_NAMESPACE_REPORT}" -P
      "${ARCHSCOPE_NAMESPACE_FILTER_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_header_filter
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_HEADER_FIXTURE}"
      "-DEXPECTED_FILE=${ARCHSCOPE_HEADER_EXPECTED}"
      "-DREPORT_OUTPUT=${ARCHSCOPE_HEADER_REPORT}" -P
      "${ARCHSCOPE_HEADER_FILTER_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_compilation_target_report
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_COMPILATION_TARGET_FIXTURE}"
      "-DEXPECTED_FILE=${ARCHSCOPE_COMPILATION_TARGET_EXPECTED}"
      "-DREPORT_OUTPUT=${ARCHSCOPE_COMPILATION_TARGET_REPORT}" -P
      "${ARCHSCOPE_COMPILATION_TARGET_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_parallel_determinism
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_PARALLEL_FIXTURE}"
      "-DREPORT_OUTPUT_DIR=${ARCHSCOPE_PARALLEL_REPORT_DIR}" -P
      "${ARCHSCOPE_PARALLEL_DETERMINISM_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_missing_compile_database
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DCOMPILE_DB=${ARCHSCOPE_MISSING_COMPILE_DB}"
      "-DMETRIC_ID=abstractness" "-DMODULE_ARG=--module=translation_unit"
      "-DEXPECTED_EXIT=3"
      "-DEXPECTED_ERROR_SUBSTRING=error: compilation database error"
      "-DEXPECTED_ERROR_SUBSTRING_2=message: missing "
      "compile_commands.json file" "-DEXPECTED_ERROR_SUBSTRING_3=path: "
      "${ARCHSCOPE_MISSING_COMPILE_DB}" -P
      "${ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_parse_failure
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DCOMPILE_DB=${ARCHSCOPE_PARSE_FAILURE_FIXTURE}"
      "-DMETRIC_ID=abstractness" "-DMODULE_ARG=--module=translation_unit"
      "-DEXPECTED_EXIT=4" "-DEXPECTED_ERROR_SUBSTRING=error: analysis error"
      "-DEXPECTED_ERROR_SUBSTRING_2=message: failed to parse translation unit"
      "-DEXPECTED_ERROR_SUBSTRING_3=translation unit: src/broken.cpp" -P
      "${ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_invalid_module_kind
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DCOMPILE_DB=${ARCHSCOPE_PLACEHOLDER_FIXTURE}"
      "-DMETRIC_ID=abstractness" "-DMODULE_ARG=--module=not_a_module"
      "-DEXPECTED_EXIT=2" "-DEXPECTED_ERROR_SUBSTRING=error: usage error"
      "-DEXPECTED_ERROR_SUBSTRING_2=message: unsupported module kind"
      "-DEXPECTED_ERROR_SUBSTRING_3=option: --module=not_a_module" -P
      "${ARCHSCOPE_ERROR_CASE_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_verbose_logging
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_PLACEHOLDER_FIXTURE}"
      "-DEXPECTED_FILE=${ARCHSCOPE_PLACEHOLDER_EXPECTED}"
      "-DREPORT_OUTPUT=${ARCHSCOPE_PLACEHOLDER_REPORT}" -P
      "${ARCHSCOPE_VERBOSE_LOGGING_CHECK_SCRIPT}")
  add_test(
    NAME archscope_system_internal_report_write_failure
    COMMAND
      ${CMAKE_COMMAND} "-DARCHSCOPE_BIN=$<TARGET_FILE:archscope>"
      "-DFIXTURE_DB=${ARCHSCOPE_PLACEHOLDER_FIXTURE}"
      "-DREPORT_OUTPUT=${ARCHSCOPE_INVALID_REPORT_OUTPUT}" -P
      "${ARCHSCOPE_INTERNAL_ERROR_CHECK_SCRIPT}")
endif()
