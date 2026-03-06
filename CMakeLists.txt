cmake_minimum_required(VERSION 3.26)

project(ArchScope VERSION 0.1.0 LANGUAGES CXX)

include(cmake/CompilerWarnings.cmake)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ARCHSCOPE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

add_library(archscope_core STATIC
    src/core/cli_text.cpp
    src/core/compilation_database.cpp
    src/core/version.cpp
)
add_library(ArchScope::core ALIAS archscope_core)

target_include_directories(archscope_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(archscope_core PUBLIC cxx_std_17)
archscope_set_common_warnings(archscope_core)

add_executable(archscope
    src/cli/main.cpp
)
target_link_libraries(archscope PRIVATE ArchScope::core)
target_compile_features(archscope PRIVATE cxx_std_17)
archscope_set_common_warnings(archscope)

add_executable(archscope_tests
    tests/support/catch_session.cpp
    tests/unit/test_main.cpp
    tests/unit/cli_text_tests.cpp
    tests/unit/compilation_database_loader_tests.cpp
    tests/unit/version_tests.cpp
)
target_include_directories(archscope_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/tests/support
)
target_link_libraries(archscope_tests PRIVATE ArchScope::core)
add_dependencies(archscope_tests archscope)
target_compile_features(archscope_tests PRIVATE cxx_std_17)
archscope_set_common_warnings(archscope_tests)

if(BUILD_TESTING)
    set(
        archscope_expected_file
        "${PROJECT_SOURCE_DIR}/tests/system/expected/version.txt"
    )
    add_test(NAME archscope_unit_tests COMMAND archscope_tests)
    add_test(
        NAME archscope_system_version
        COMMAND ${CMAKE_COMMAND}
            -DARCHSCOPE_BIN=$<TARGET_FILE:archscope>
            -DEXPECTED_FILE=${archscope_expected_file}
            -P ${PROJECT_SOURCE_DIR}/tests/system/check_version.cmake
    )
endif()
