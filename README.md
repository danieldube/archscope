# ArchScope

ArchScope is a C++17 command-line tool for computing architecture metrics over
projects that provide a `compile_commands.json` database. The current codebase
includes the bootstrap CLI, a tested compilation-database loader, and the first
placeholder Markdown report flow for translation-unit modules.

## Quick start

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit run --all-files
./build/archscope --help
./build/archscope --version
./build/archscope tests/fixtures/placeholder_project/compile_commands.json \
  abstractness --module=translation_unit --report=/tmp/archscope-report.md
```

## Current status

The CLI now accepts a compilation database path, one or more metric ids, and
`--module=translation_unit`, then writes a deterministic Markdown report with
placeholder `0.000` metric values. Real metric computation and Clang-based
analysis remain for later roadmap increments.

## Repository layout

- `src/cli/`: executable entrypoint.
- `src/core/`: reusable logic shared by the executable and tests, including
  compilation-database loading and Markdown report rendering.
- `src/clang/`: reserved for Clang LibTooling integration.
- `src/report/`: reserved for report generation.
- `tests/unit/`: fast unit coverage.
- `tests/system/`: CTest-driven system coverage.
- `docs/`: user, developer, and ADR documentation.
