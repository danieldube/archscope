# Developer Guide

## Build prerequisites

- CMake 3.26+
- A C++17 compiler
- Ninja
- LLVM/Clang development packages with LibTooling support
- `libcurl` development headers and libraries
- `pre-commit`
- `clang-format`
- `clang-tidy`
- Network access for the first test-enabled configure, unless Catch2 is
  provided through a pre-populated CMake download cache or internal mirror

On Ubuntu 24.04, the CI-tested package set is:

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  clang-format \
  clang-tidy \
  libclang-18-dev \
  libcurl4-openssl-dev \
  llvm-18-dev \
  ninja-build
```

## Local workflows

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit install
pre-commit run --all-files
```

`pre-commit` runs both formatting checks and `clang-tidy`. The `clang-tidy`
hook uses `build/compile_commands.json`, so run `./scripts/configure.sh` before
invoking it.

If CMake cannot find Clang tooling automatically, point it at the desired LLVM
install by setting `ARCHSCOPE_LLVM_VERSION`, for example:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DARCHSCOPE_LLVM_VERSION=18
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
- `src/core/metrics.*` now provides the pure abstractness computation used by
  unit tests; later increments will add classification, registry wiring, and
  additional metrics on top of it.
- `src/clang/tool_runner.*` adapts the loaded compilation database into
  `clang::tooling::ClangTool` runs and returns deterministic plain-C++ extracted
  type records, including whether each definition is abstract.
- `archscope_tests` links against `Catch2::Catch2WithMain`, and CTest
  registration uses `catch_discover_tests`.
- It supports the standard compilation database entry shapes:
  `directory` + `file` + either `arguments` or `command`.
- `CompilationDatabase::translation_unit_paths()` returns a sorted list so
  higher layers can stay deterministic.
- The CLI computes translation-unit `abstractness` from extracted types.
  `instability` and `distance_from_main_sequence` remain placeholder values
  until later roadmap tasks wire those metrics in.
- The Clang extraction layer currently collects class/struct definitions,
  definition file paths, and qualified names while excluding forward
  declarations and system-header types.

## Next implementation steps

The next increment wires the extracted types into translation-unit module
ownership and then replaces placeholder metric values incrementally.
