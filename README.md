# ArchScope

ArchScope is a C++17 command-line tool for computing architecture metrics over
projects that provide a `compile_commands.json` database. This increment
bootstraps the repository with a runnable `archscope` executable, unit tests,
system tests, and CI-quality guardrails.

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

The bootstrap CLI only supports `--help` and `--version`. Compilation-database
loading, analysis, and Markdown report generation arrive in later roadmap
increments.

## Repository layout

- `src/cli/`: executable entrypoint.
- `src/core/`: reusable logic shared by the executable and tests.
- `src/clang/`: reserved for Clang LibTooling integration.
- `src/report/`: reserved for report generation.
- `tests/unit/`: fast unit coverage.
- `tests/system/`: CTest-driven system coverage.
- `docs/`: user, developer, and ADR documentation.
