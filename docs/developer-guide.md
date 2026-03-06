# Developer Guide

## Build prerequisites

- CMake 3.26+
- A C++17 compiler
- Ninja
- `pre-commit`
- `clang-format`
- `clang-tidy`
- Network access for the first test-enabled configure, unless Catch2 is
  provided through a pre-populated CMake download cache or internal mirror

## Local workflows

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit install
pre-commit run --all-files
./scripts/run-tidy.sh
```

## Test layout

- `tests/unit/` holds fast tests for library code.
- `tests/system/` holds CTest-driven executable checks.
- Unit tests use Catch2 v3 fetched with CMake `FetchContent` during configure.
- `tests/fixtures/placeholder_project/` provides the Increment 1 translation
  unit fixture for placeholder report generation.

## Current implementation notes

- `src/core/compilation_database.*` provides the current plain-C++ loader for
  `compile_commands.json`.
- `src/core/report.*` owns the current report model and deterministic Markdown
  rendering.
- `archscope_tests` links against `Catch2::Catch2WithMain`, and CTest
  registration uses `catch_discover_tests`.
- It supports the standard compilation database entry shapes:
  `directory` + `file` + either `arguments` or `command`.
- `CompilationDatabase::translation_unit_paths()` returns a sorted list so
  higher layers can stay deterministic.
- The CLI currently supports placeholder output for
  `--module=translation_unit`; it validates metric ids and writes `0.000`
  values in request order.

## Next implementation steps

The next increments will replace the placeholder metric values with Clang
LibTooling-backed extraction, starting with enumerating types per translation
unit while preserving the current target layout.
