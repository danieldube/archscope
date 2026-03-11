# ArchScope

ArchScope is a C++17 command-line tool for computing architecture metrics over
projects that provide a `compile_commands.json` database. The current codebase
includes the bootstrap CLI, a tested compilation-database loader, a Clang
LibTooling extraction layer for C++ type and dependency discovery, and a
deterministic Markdown report flow for translation-unit, namespace, and header
modules.

## Quick start

Install the required system packages first. On Ubuntu 24.04:

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

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit run --all-files
./build/archscope --help
./build/archscope --version
./build/archscope tests/fixtures/placeholder_project/compile_commands.json \
  abstractness --module=translation_unit --report=/tmp/archscope-report.md
./build/archscope tests/fixtures/namespace_project/compile_commands.json \
  abstractness instability --module=namespace --module-filter=a::b \
  --report=/tmp/archscope-namespace-report.md
./build/archscope tests/fixtures/parallel_project/compile_commands.json \
  abstractness instability distance_from_main_sequence \
  --module=translation_unit --threads=4 \
  --report=/tmp/archscope-deterministic-report.md
```

## Current status

The CLI now accepts a compilation database path, one or more metric ids, and
`--module=translation_unit`, `--module=namespace`, or `--module=header`, then writes a
deterministic Markdown report. `abstractness`, `instability`, and
`distance_from_main_sequence` are wired through the metric registry and emitted
in requested CLI order. `archscope_clang` parses translation units with Clang
LibTooling, enumerates user-defined class/struct definitions with qualified
names, definition paths, namespace owners, and abstract/concrete
classification, and extracts dependency candidates from base classes, fields,
and function signatures. `--module-filter` now filters output modules without
changing the full analysis pass; namespace filters use prefix matching.
Header ownership is now wired through the analysis path: header modules are
owned by the definition spelling file, system headers are still excluded, and
header-mode dependency edges use definition paths instead of translation-unit
paths. Header filtering is now covered end to end as well: `--module=header`
uses substring matching on normalized paths, so relative inputs with `..`
segments still match the emitted absolute header-module paths. The CLI now also
accepts `--threads=<n>`, clamps it to the available TU count, and now runs
per-translation-unit extraction in parallel with deterministic merged output.
Determinism coverage uses a larger multi-TU fixture and compares repeated
`--threads=1` and `--threads=4` runs for translation-unit, namespace, and
header reports.

## Repository layout

- `src/cli/`: executable entrypoint.
- `src/core/`: reusable logic shared by the executable and tests, including
  compilation-database loading and Markdown report rendering.
- `src/clang/`: Clang LibTooling integration that extracts plain C++ type data.
- `src/report/`: reserved for report generation.
- `tests/unit/`: fast unit coverage.
- `tests/system/`: CTest-driven system coverage.
- `docs/`: user, developer, and ADR documentation.
