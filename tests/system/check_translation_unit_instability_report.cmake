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

execute_process(
  COMMAND
    "${ARCHSCOPE_BIN}" "${FIXTURE_DB}" instability --module=translation_unit
    --project-name=dependency_fixture "--report=${REPORT_OUTPUT}"
  RESULT_VARIABLE archscope_result
  OUTPUT_VARIABLE archscope_output
  ERROR_VARIABLE archscope_error
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT archscope_result EQUAL 0)
  message(
    FATAL_ERROR "archscope instability run failed with ${archscope_result}: "
                "${archscope_error}")
endif()

if(NOT archscope_output STREQUAL "")
  message(
    FATAL_ERROR "Expected no stdout output but got '${archscope_output}'")
endif()

file(READ "${EXPECTED_FILE}" expected_output)
file(READ "${REPORT_OUTPUT}" actual_output)

if(NOT actual_output STREQUAL expected_output)
  message(
    FATAL_ERROR "Generated instability report did not match expected output.")
endif()
