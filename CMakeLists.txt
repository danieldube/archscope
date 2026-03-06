cmake_minimum_required(VERSION 3.26)

project(ArchScope VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
include(cmake/CompilerWarnings.cmake)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ARCHSCOPE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

if(BUILD_TESTING)
    set(archscope_local_catch2_dirs
        "${PROJECT_BINARY_DIR}/_deps/catch2-src"
        "${PROJECT_SOURCE_DIR}/cmake-build-debug/_deps/catch2-src"
    )
    foreach(archscope_catch2_dir IN LISTS archscope_local_catch2_dirs)
        if(EXISTS "${archscope_catch2_dir}/CMakeLists.txt")
            set(FETCHCONTENT_SOURCE_DIR_CATCH2 "${archscope_catch2_dir}")
            break()
        endif()
    endforeach()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
    include(Catch)
endif()

add_library(archscope_core STATIC
    src/core/analysis.cpp
    src/core/cli_text.cpp
    src/core/compilation_database.cpp
    src/core/report.cpp
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

if(BUILD_TESTING)
    add_executable(archscope_tests
        tests/unit/analysis_result_tests.cpp
        tests/unit/cli_text_tests.cpp
        tests/unit/compilation_database_loader_tests.cpp
        tests/unit/report_writer_tests.cpp
        tests/unit/version_tests.cpp
    )
    target_include_directories(archscope_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )
    target_link_libraries(
        archscope_tests
        PRIVATE
            ArchScope::core
            Catch2::Catch2WithMain
    )
    add_dependencies(archscope_tests archscope)
    target_compile_features(archscope_tests PRIVATE cxx_std_17)
    archscope_set_common_warnings(archscope_tests)

    set(
        archscope_expected_file
        "${PROJECT_SOURCE_DIR}/tests/system/expected/version.txt"
    )
    set(
        archscope_placeholder_fixture
        "${PROJECT_SOURCE_DIR}/tests/fixtures/placeholder_project/compile_commands.json"
    )
    set(
        archscope_placeholder_expected
        "${PROJECT_SOURCE_DIR}/tests/system/expected/placeholder-report.md"
    )
    set(
        archscope_placeholder_report
        "${PROJECT_BINARY_DIR}/tests/system/placeholder-report.md"
    )
    catch_discover_tests(archscope_tests)
    add_test(
        NAME archscope_system_version
        COMMAND ${CMAKE_COMMAND}
            -DARCHSCOPE_BIN=$<TARGET_FILE:archscope>
            -DEXPECTED_FILE=${archscope_expected_file}
            -P ${PROJECT_SOURCE_DIR}/tests/system/check_version.cmake
    )
    add_test(
        NAME archscope_system_placeholder_report
        COMMAND ${CMAKE_COMMAND}
            -DARCHSCOPE_BIN=$<TARGET_FILE:archscope>
            -DFIXTURE_DB=${archscope_placeholder_fixture}
            -DEXPECTED_FILE=${archscope_placeholder_expected}
            -DREPORT_OUTPUT=${archscope_placeholder_report}
            -P ${PROJECT_SOURCE_DIR}/tests/system/check_placeholder_report.cmake
    )
endif()
