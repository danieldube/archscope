# ArchScope

ArchScope is a C++17 command-line tool for computing architecture metrics over
projects that provide a `compile_commands.json` database. The current codebase
includes the bootstrap CLI plus the first compilation-database loading
contract, covered by unit tests for valid input, missing files, invalid JSON,
and compile-argument extraction. Unit tests now use Catch2 v3 fetched by CMake
at configure time.

## Quick start

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit run --all-files
./build/archscope --help
./build/archscope --version
```

## Current status

The CLI still only supports `--help` and `--version`, but `archscope_core` now
contains a tested `compile_commands.json` loader that the later analysis
pipeline will build on. Markdown report generation and metric analysis remain
for later roadmap increments.

## Repository layout

- `src/cli/`: executable entrypoint.
- `src/core/`: reusable logic shared by the executable and tests.
- `src/clang/`: reserved for Clang LibTooling integration.
- `src/report/`: reserved for report generation.
- `tests/unit/`: fast unit coverage.
- `tests/system/`: CTest-driven system coverage.
- `docs/`: user, developer, and ADR documentation.
