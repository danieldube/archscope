# Developer Guide

## Build prerequisites

- CMake 3.26+
- A C++17 compiler
- Ninja
- `pre-commit`
- `clang-format`
- `clang-tidy`

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
- The current test harness includes a minimal local `catch2`-compatible shim
  because external package download is intentionally avoided during bootstrap.
- The next analysis pipeline pieces should extend the existing
  `compilation_database_loader_tests.cpp` contract before wiring new CLI or
  Clang-tooling behavior on top.

## Current implementation notes

- `src/core/compilation_database.*` provides the current plain-C++ loader for
  `compile_commands.json`.
- It supports the standard compilation database entry shapes:
  `directory` + `file` + either `arguments` or `command`.
- `CompilationDatabase::translation_unit_paths()` returns a sorted list so
  higher layers can stay deterministic.

## Next implementation steps

The next increments will build on the current loader by integrating Clang
LibTooling, extracting types per translation unit, and adding Markdown report
generation while preserving the current target layout.
