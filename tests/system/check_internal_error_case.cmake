if(NOT DEFINED ARCHSCOPE_BIN)
  message(FATAL_ERROR "ARCHSCOPE_BIN is required")
endif()

if(NOT DEFINED FIXTURE_DB)
  message(FATAL_ERROR "FIXTURE_DB is required")
endif()

if(NOT DEFINED REPORT_OUTPUT)
  message(FATAL_ERROR "REPORT_OUTPUT is required")
endif()

execute_process(
  COMMAND "${ARCHSCOPE_BIN}" "${FIXTURE_DB}" abstractness
          --module=translation_unit "--report=${REPORT_OUTPUT}"
  RESULT_VARIABLE archscope_result
  OUTPUT_VARIABLE archscope_output
  ERROR_VARIABLE archscope_error
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)

if(NOT archscope_result EQUAL 5)
  message(
    FATAL_ERROR "Expected exit code 5 but got ${archscope_result}. stderr: "
                "${archscope_error}")
endif()

if(NOT archscope_output STREQUAL "")
  message(
    FATAL_ERROR "Expected no stdout output but got '${archscope_output}'")
endif()

foreach(
  expected_line IN
  ITEMS "error: internal error" "message: failed to create report directory"
        "report: ${REPORT_OUTPUT}")
  string(FIND "${archscope_error}" "${expected_line}" error_match)
  if(error_match EQUAL -1)
    message(
      FATAL_ERROR "Expected stderr to contain '${expected_line}' but got "
                  "'${archscope_error}'")
  endif()
endforeach()
