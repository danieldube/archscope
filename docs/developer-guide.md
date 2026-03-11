# Developer Guide

## Architecture

ArchScope keeps the production code split into three layers:

- `archscope` in `src/cli/`: argument parsing, exit-code mapping, report file
  IO, and top-level orchestration
- `archscope_core` in `src/core/`: plain C++ domain model, metric functions,
  module filtering, report rendering, and compile-database loading
- `archscope_clang` in `src/clang/`: Clang LibTooling execution, AST traversal,
  and projection from AST data into the core domain model

`archscope_core` must stay free of Clang headers. Any AST-dependent logic
belongs in `src/clang/` and should be translated back into plain C++ data
before reaching the core layer.

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

If CMake cannot find Clang tooling automatically, point it at the desired LLVM
install by setting `ARCHSCOPE_LLVM_VERSION`, for example:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DARCHSCOPE_LLVM_VERSION=18
```

## Local workflows

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit install
pre-commit run --all-files
```

`pre-commit` runs formatting checks and `clang-tidy`. The `clang-tidy` hook
uses `build/compile_commands.json`, so run `./scripts/configure.sh` first.
Run build and test serially against the same build directory so CTest does not
observe stale binaries.

## Test strategy

- `tests/unit/` holds fast tests for library code.
- `tests/system/` holds CTest-driven executable checks.
- Unit tests use Catch2 v3 fetched with CMake `FetchContent` during configure.
- System fixtures live under `tests/fixtures/` and are committed so the suite
  stays deterministic and offline-friendly.
- New behavior should start with the smallest failing unit or system test that
  proves the acceptance criteria for the current roadmap task.

## Current implementation notes

- `src/core/compilation_database.*` provides the plain-C++ loader for
  `compile_commands.json`.
- `src/core/report.*` owns the report model and deterministic Markdown
  rendering.
- `src/core/metrics.*` provides `MetricRegistry` plus pure computations for
  abstractness, instability, abstract/concrete/total type counts, and
  distance from the main sequence.
- `src/core/module_filter.*` owns module-name matching rules so report
  filtering stays independent from Clang traversal and metric computation.
- `src/core/compilation_database.*` also derives a best-effort
  `compilation_target` id from each compile command's output metadata so the
  analysis layer can group translation units by binary or library target.
- `src/clang/tool_runner.*` adapts the loaded compilation database into
  `clang::tooling::ClangTool` runs and returns deterministic plain-C++ extracted
  type records plus dependency candidates derived from base classes, fields,
  and function signatures.
- `src/clang/analysis_projection.*` projects extracted data into
  `AnalysisResult` for the currently supported module kinds.
- `archscope_tests` links against `Catch2::Catch2WithMain`, and CTest
  registration uses `catch_discover_tests`.
- Relative `directory` values in `compile_commands.json` are resolved from the
  directory that contains the database, so checked-in test fixtures remain
  runnable without per-test rewrites.
- `CompilationDatabase::translation_unit_paths()` returns a sorted list so
  higher layers can stay deterministic.
- `--module-filter` is applied after analysis when building the report model,
  so metrics still reflect the full analyzed project graph.
- `src/cli/main.cpp` parses `--threads=<n>` and passes the clamped value into
  the Clang extraction layer.
- `src/clang/tool_runner.*` runs independent translation units on a small
  worker pool, stores per-entry results in compile-database order, and merges
  them after all workers finish so the final extracted types and dependencies
  remain deterministic.
- Compilation-target ownership is intentionally per compile command. If the
  same definition is compiled into multiple targets, it can appear once per
  target in `--module=compilation_target` output.

## Adding a new metric

1. Add unit coverage in `tests/unit/` for the pure metric behavior and edge
   cases.
2. Extend `src/core/metrics.hpp` and `src/core/metrics.cpp` with the new
   `MetricId`, display name, computation function, and any metric-specific
   report formatting rules.
3. Register the metric in `MetricRegistry::with_defaults()` so the CLI can
   resolve it in request order.
4. Update or add a system test fixture if end-to-end report output changes.
5. Update `docs/user-manual.md` and `README.md` with the new metric id and an
   example invocation.

Metric functions should stay pure: they consume `AnalysisResult` plus
`ModuleId`, and they do not perform filesystem access or Clang queries.

## Adding a new module kind

1. Add focused unit coverage for the new ownership rule in
   `tests/unit/analysis_projection_tests.cpp`, plus filter and CLI/help
   coverage if the public contract changes.
2. Extend the plain-C++ source data first if the new module kind needs extra
   compile-database metadata or extraction fields.
3. Keep ownership derivation deterministic and document any fallback behavior
   in `docs/adr/0006-module-ownership-rules.md`.
4. Add or extend a fixture-backed system test so the new module kind is
   exercised end to end and in the parallel determinism matrix.

## Adding a new dependency extraction rule

1. Add a focused unit test in `tests/unit/clang_tool_runner_tests.cpp` or a new
   Clang-facing test that proves the new reference kind is discovered.
2. Implement the AST walk in `src/clang/tool_runner.cpp` or its helper types,
   keeping the new logic inside the Clang adapter layer.
3. Project the extracted dependency target through
   `src/clang/analysis_projection.*` only if the core model needs new plain-C++
   fields.
4. Extend system coverage with a minimal fixture if the rule changes emitted
   `Ce`, `Ca`, `I`, or `D` values.
5. Document the extraction rule and exclusions in `docs/adr/`.

Before accepting the change, verify that system headers, self-dependencies, and
duplicate edges are still excluded.

## Key files

- `src/cli/main.cpp`: CLI contract, exit-code handling, orchestration
- `src/core/metrics.cpp`: metric registry and pure metric computations
- `src/core/module_filter.cpp`: module-kind-specific filtering semantics
- `src/core/report.cpp`: deterministic Markdown rendering
- `src/clang/tool_runner.cpp`: Clang extraction and dependency collection
- `src/clang/analysis_projection.cpp`: ownership mapping and graph assembly
