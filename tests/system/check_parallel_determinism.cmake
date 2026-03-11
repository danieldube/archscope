if(NOT DEFINED ARCHSCOPE_BIN)
  message(FATAL_ERROR "ARCHSCOPE_BIN is required")
endif()

if(NOT DEFINED FIXTURE_DB)
  message(FATAL_ERROR "FIXTURE_DB is required")
endif()

if(NOT DEFINED REPORT_OUTPUT_DIR)
  message(FATAL_ERROR "REPORT_OUTPUT_DIR is required")
endif()

# Run archscope for one module/thread combination and require quiet success.
function(run_archscope module_kind threads iteration output_file)
  execute_process(
    COMMAND
      "${ARCHSCOPE_BIN}" "${FIXTURE_DB}" abstractness instability
      distance_from_main_sequence "--module=${module_kind}"
      "--threads=${threads}" "--project-name=determinism_fixture"
      "--report=${output_file}"
    RESULT_VARIABLE ARCHSCOPE_RESULT
    OUTPUT_VARIABLE ARCHSCOPE_OUTPUT
    ERROR_VARIABLE ARCHSCOPE_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(NOT ARCHSCOPE_RESULT EQUAL 0)
    message(
      FATAL_ERROR
        "archscope run failed for module=${module_kind}, "
        "threads=${threads}, iteration=${iteration}: " "${ARCHSCOPE_ERROR}")
  endif()

  if(NOT ARCHSCOPE_OUTPUT STREQUAL "")
    message(
      FATAL_ERROR
        "Expected no stdout output for module=${module_kind}, "
        "threads=${threads}, iteration=${iteration}, but got "
        "'${ARCHSCOPE_OUTPUT}'")
  endif()
endfunction()

set(ARCHSCOPE_MODULE_KINDS translation_unit namespace header
                           compilation_target)

foreach(module_kind IN LISTS ARCHSCOPE_MODULE_KINDS)
  set(REFERENCE_OUTPUT
      "${REPORT_OUTPUT_DIR}/${module_kind}-threads-1-reference.md")
  run_archscope("${module_kind}" 1 "reference" "${REFERENCE_OUTPUT}")

  foreach(iteration IN ITEMS 1 2 3)
    set(SERIAL_OUTPUT
        "${REPORT_OUTPUT_DIR}/${module_kind}-threads-1-${iteration}.md")
    run_archscope("${module_kind}" 1 "${iteration}" "${SERIAL_OUTPUT}")

    file(READ "${REFERENCE_OUTPUT}" REFERENCE_CONTENTS)
    file(READ "${SERIAL_OUTPUT}" SERIAL_CONTENTS)
    if(NOT SERIAL_CONTENTS STREQUAL REFERENCE_CONTENTS)
      message(FATAL_ERROR "Serial rerun produced different output for "
                          "module=${module_kind}, iteration=${iteration}.")
    endif()

    set(PARALLEL_OUTPUT
        "${REPORT_OUTPUT_DIR}/${module_kind}-threads-4-${iteration}.md")
    run_archscope("${module_kind}" 4 "${iteration}" "${PARALLEL_OUTPUT}")

    file(READ "${PARALLEL_OUTPUT}" PARALLEL_CONTENTS)
    if(NOT PARALLEL_CONTENTS STREQUAL REFERENCE_CONTENTS)
      message(FATAL_ERROR "Parallel run produced different output for "
                          "module=${module_kind}, iteration=${iteration}.")
    endif()
  endforeach()
endforeach()

file(READ "${REPORT_OUTPUT_DIR}/namespace-threads-1-reference.md"
     NAMESPACE_OUTPUT)
if(NAMESPACE_OUTPUT MATCHES "platform::domain:"
   AND NAMESPACE_OUTPUT MATCHES "platform::app:"
   AND NAMESPACE_OUTPUT MATCHES "platform::adapters:")
  set(NAMESPACE_MODULES_PRESENT TRUE)
else()
  set(NAMESPACE_MODULES_PRESENT FALSE)
endif()

if(NOT NAMESPACE_MODULES_PRESENT)
  message(
    FATAL_ERROR
      "Namespace determinism fixture did not emit the expected modules.")
endif()

file(READ "${REPORT_OUTPUT_DIR}/header-threads-1-reference.md" HEADER_OUTPUT)
if(NOT HEADER_OUTPUT MATCHES "include/platform/api.hpp:"
   OR NOT HEADER_OUTPUT MATCHES "src/domain/order.cpp:"
   OR NOT HEADER_OUTPUT MATCHES "src/app/facade.cpp:")
  message(FATAL_ERROR "Header determinism fixture did not emit the expected "
                      "header/source owner modules.")
endif()

file(READ "${REPORT_OUTPUT_DIR}/compilation_target-threads-1-reference.md"
     COMPILATION_TARGET_OUTPUT)
if(NOT COMPILATION_TARGET_OUTPUT MATCHES "platform_adapters:"
   OR NOT COMPILATION_TARGET_OUTPUT MATCHES "platform_app:"
   OR NOT COMPILATION_TARGET_OUTPUT MATCHES "platform_domain:")
  message(
    FATAL_ERROR "Compilation-target determinism fixture did not emit the "
                "expected target owner modules.")
endif()
