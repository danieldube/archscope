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
- `src/core/metrics.*` now provides `MetricRegistry` plus pure computations for
  abstractness, instability, and distance from the main sequence.
- `src/core/module_filter.*` owns pure module-name matching rules so report
  filtering stays independent from Clang traversal and metric computation.
- `src/clang/tool_runner.*` adapts the loaded compilation database into
  `clang::tooling::ClangTool` runs and returns deterministic plain-C++ extracted
  type records plus dependency candidates derived from base classes, fields,
  and function signatures.
- `src/clang/analysis_projection.*` projects extracted data into
  `AnalysisResult` for the currently supported module kinds.
- `archscope_tests` links against `Catch2::Catch2WithMain`, and CTest
  registration uses `catch_discover_tests`.
- It supports the standard compilation database entry shapes:
  `directory` + `file` + either `arguments` or `command`.
- Relative `directory` values in `compile_commands.json` are resolved from the
  directory that contains the database, so checked-in test fixtures remain
  runnable without per-test rewrites.
- `CompilationDatabase::translation_unit_paths()` returns a sorted list so
  higher layers can stay deterministic.
- The CLI requests metric computation through `MetricRegistry`, preserving user
  metric order in the emitted report.
- The Clang extraction layer currently collects class/struct definitions,
  definition file paths, namespace owners, qualified names, and outgoing
  dependency candidates while excluding forward declarations and system-header
  types from the type inventory.
- Header ownership is now implemented by carrying definition spelling paths
  through both extracted types and dependencies, while translation-unit
  projection skips only dependency targets that have no owning TU entry.
- `--module-filter` is applied after analysis when building the report model,
  so metrics still reflect the full analyzed project graph.
- Header filters normalize both the emitted module path and the filter text
  with `std::filesystem::path::lexically_normal()` before performing substring
  matching.
- `src/cli/main.cpp` parses `--threads=<n>` and passes the clamped value into
  the Clang extraction layer.
- `tests/system/check_parallel_determinism.cmake` now drives a larger
  `tests/fixtures/parallel_project/` fixture and compares repeated report bytes
  from `--threads=1` and `--threads=4` for translation-unit, namespace, and
  header modes, which gives Task 8.2 good coverage for aggregation and ordering
  races.
- `src/clang/tool_runner.*` now runs independent translation units on a small
  worker pool, stores per-entry results in compile-database order, and merges
  them after all workers finish so the final extracted types and dependencies
  remain deterministic.

## Next implementation steps

The next open increment is Increment 9: documentation and ADR completion.
