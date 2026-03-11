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
    "${ARCHSCOPE_BIN}" "${FIXTURE_DB}" instability abstractness
    --module=translation_unit --project-name=placeholder_project --verbose
    "--report=${REPORT_OUTPUT}"
  RESULT_VARIABLE archscope_result
  OUTPUT_VARIABLE archscope_output
  ERROR_VARIABLE archscope_error
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)

if(NOT archscope_result EQUAL 0)
  message(FATAL_ERROR "Verbose archscope run failed with ${archscope_result}: "
                      "${archscope_error}")
endif()

if(NOT archscope_output STREQUAL "")
  message(
    FATAL_ERROR "Expected no stdout output but got '${archscope_output}'")
endif()

foreach(
  expected_line IN
  ITEMS "info: loading compilation database from"
        "info: loaded 2 compilation database entries"
        "info: analyzing 2 translation units with"
        "info: writing Markdown report to"
        "info: report written successfully")
  string(FIND "${archscope_error}" "${expected_line}" verbose_match)
  if(verbose_match EQUAL -1)
    message(
      FATAL_ERROR
        "Expected verbose stderr to contain '${expected_line}' but got "
        "'${archscope_error}'")
  endif()
endforeach()

file(READ "${EXPECTED_FILE}" expected_output)
file(READ "${REPORT_OUTPUT}" actual_output)

if(NOT actual_output STREQUAL expected_output)
  message(FATAL_ERROR "Verbose run report did not match expected output.")
endif()
