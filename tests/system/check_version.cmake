if(NOT DEFINED ARCHSCOPE_BIN)
    message(FATAL_ERROR "ARCHSCOPE_BIN is required")
endif()

if(NOT DEFINED EXPECTED_FILE)
    message(FATAL_ERROR "EXPECTED_FILE is required")
endif()

execute_process(
    COMMAND "${ARCHSCOPE_BIN}" --version
    RESULT_VARIABLE archscope_result
    OUTPUT_VARIABLE archscope_output
    ERROR_VARIABLE archscope_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT archscope_result EQUAL 0)
    set(
        version_failure
        "archscope --version failed with ${archscope_result}: "
        "${archscope_error}"
    )
    message(
        FATAL_ERROR
        "${version_failure}"
    )
endif()

file(READ "${EXPECTED_FILE}" expected_output)
string(STRIP "${expected_output}" expected_output)

if(NOT archscope_output STREQUAL expected_output)
    message(
        FATAL_ERROR
        "Expected '${expected_output}' but got '${archscope_output}'"
    )
endif()
