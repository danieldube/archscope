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
- The next analysis pipeline pieces should extend the existing
  `compilation_database_loader_tests.cpp` contract before wiring new CLI or
  Clang-tooling behavior on top.

## Current implementation notes

- `src/core/compilation_database.*` provides the current plain-C++ loader for
  `compile_commands.json`.
- `archscope_tests` links against `Catch2::Catch2WithMain`, and CTest
  registration uses `catch_discover_tests`.
- It supports the standard compilation database entry shapes:
  `directory` + `file` + either `arguments` or `command`.
- `CompilationDatabase::translation_unit_paths()` returns a sorted list so
  higher layers can stay deterministic.

## Next implementation steps

The next increments will build on the current loader by integrating Clang
LibTooling, extracting types per translation unit, and adding Markdown report
generation while preserving the current target layout.
