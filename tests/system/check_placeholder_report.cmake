if(NOT DEFINED ARCHSCOPE_BIN)
  message(FATAL_ERROR "ARCHSCOPE_BIN is required")
endif()

if(NOT DEFINED FIXTURE_DB)
  message(FATAL_ERROR "FIXTURE_DB is required")
endif()

if(NOT DEFINED EXPECTED_FILE)
  message(FATAL_ERROR "EXPECTED_FILE is required")
endif()

if(NOT DEFINED REPORT_OUTPUT)
  message(FATAL_ERROR "REPORT_OUTPUT is required")
endif()

get_filename_component(FIXTURE_DIR "${FIXTURE_DB}" DIRECTORY)
set(RUNTIME_COMPILE_DB "${REPORT_OUTPUT}.compile_commands.json")

string(
  CONCAT RUNTIME_COMPILE_DB_CONTENT
         "[\n"
         "  {\n"
         "    \"directory\": \""
         "${FIXTURE_DIR}"
         "\",\n"
         "    \"file\": \"src/zeta.cpp\",\n"
         "    \"arguments\": [\n"
         "      \"clang++\",\n"
         "      \"-std=c++17\",\n"
         "      \"src/zeta.cpp\"\n"
         "    ]\n"
         "  },\n"
         "  {\n"
         "    \"directory\": \""
         "${FIXTURE_DIR}"
         "\",\n"
         "    \"file\": \"src/alpha.cpp\",\n"
         "    \"arguments\": [\n"
         "      \"clang++\",\n"
         "      \"-std=c++17\",\n"
         "      \"src/alpha.cpp\"\n"
         "    ]\n"
         "  }\n"
         "]\n")
file(WRITE "${RUNTIME_COMPILE_DB}" "${RUNTIME_COMPILE_DB_CONTENT}")

execute_process(
  COMMAND
    "${ARCHSCOPE_BIN}" "${RUNTIME_COMPILE_DB}" abstractness
    --module=translation_unit --project-name=placeholder_project
    "--report=${REPORT_OUTPUT}"
  RESULT_VARIABLE archscope_result
  OUTPUT_VARIABLE archscope_output
  ERROR_VARIABLE archscope_error
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT archscope_result EQUAL 0)
  message(
    FATAL_ERROR "archscope placeholder run failed with ${archscope_result}: "
                "${archscope_error}")
endif()

if(NOT archscope_output STREQUAL "")
  message(FATAL_ERROR "Expected no stdout output but got '${archscope_output}'")
endif()

file(READ "${EXPECTED_FILE}" expected_output)
file(READ "${REPORT_OUTPUT}" actual_output)

if(NOT actual_output STREQUAL expected_output)
  message(FATAL_ERROR "Generated report did not match expected output.")
endif()
