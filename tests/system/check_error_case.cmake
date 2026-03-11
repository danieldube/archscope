if(NOT DEFINED ARCHSCOPE_BIN)
  message(FATAL_ERROR "ARCHSCOPE_BIN is required")
endif()

if(NOT DEFINED COMPILE_DB)
  message(FATAL_ERROR "COMPILE_DB is required")
endif()

if(NOT DEFINED METRIC_ID)
  message(FATAL_ERROR "METRIC_ID is required")
endif()

if(NOT DEFINED MODULE_ARG)
  message(FATAL_ERROR "MODULE_ARG is required")
endif()

if(NOT DEFINED EXPECTED_EXIT)
  message(FATAL_ERROR "EXPECTED_EXIT is required")
endif()

if(NOT DEFINED EXPECTED_ERROR_SUBSTRING)
  message(FATAL_ERROR "EXPECTED_ERROR_SUBSTRING is required")
endif()

execute_process(
  COMMAND "${ARCHSCOPE_BIN}" "${COMPILE_DB}" "${METRIC_ID}" "${MODULE_ARG}"
  RESULT_VARIABLE archscope_result
  OUTPUT_VARIABLE archscope_output
  ERROR_VARIABLE archscope_error
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)

if(NOT archscope_result EQUAL EXPECTED_EXIT)
  message(
    FATAL_ERROR
      "Expected exit code ${EXPECTED_EXIT} but got ${archscope_result}. "
      "stderr: ${archscope_error}")
endif()

if(NOT archscope_output STREQUAL "")
  message(
    FATAL_ERROR "Expected no stdout output but got '${archscope_output}'")
endif()

string(FIND "${archscope_error}" "${EXPECTED_ERROR_SUBSTRING}"
            archscope_error_match)
if(archscope_error_match EQUAL -1)
  message(
    FATAL_ERROR
      "Expected stderr to contain '${EXPECTED_ERROR_SUBSTRING}' but got "
      "'${archscope_error}'")
endif()

if(DEFINED EXPECTED_ERROR_SUBSTRING_2)
  string(FIND "${archscope_error}" "${EXPECTED_ERROR_SUBSTRING_2}"
              archscope_error_match_2)
  if(archscope_error_match_2 EQUAL -1)
    message(
      FATAL_ERROR
        "Expected stderr to contain '${EXPECTED_ERROR_SUBSTRING_2}' but got "
        "'${archscope_error}'")
  endif()
endif()

if(DEFINED EXPECTED_ERROR_SUBSTRING_3)
  string(FIND "${archscope_error}" "${EXPECTED_ERROR_SUBSTRING_3}"
              archscope_error_match_3)
  if(archscope_error_match_3 EQUAL -1)
    message(
      FATAL_ERROR
        "Expected stderr to contain '${EXPECTED_ERROR_SUBSTRING_3}' but got "
        "'${archscope_error}'")
  endif()
endif()
