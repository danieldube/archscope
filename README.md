# ArchScope

ArchScope is a C++17 command-line tool for computing architecture metrics over
projects that provide a `compile_commands.json` database. The current codebase
includes the bootstrap CLI, a tested compilation-database loader, a Clang
LibTooling extraction layer for C++ type discovery, and the placeholder
Markdown report flow for translation-unit modules.

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
placeholder `0.000` metric values. Under the hood, `archscope_clang` can now
parse translation units with Clang LibTooling and enumerate user-defined
class/struct definitions with qualified names and definition paths. Metric
computation and CLI integration for extracted types remain for later roadmap
increments.

## Repository layout

- `src/cli/`: executable entrypoint.
- `src/core/`: reusable logic shared by the executable and tests, including
  compilation-database loading and Markdown report rendering.
- `src/clang/`: Clang LibTooling integration that extracts plain C++ type data.
- `src/report/`: reserved for report generation.
- `tests/unit/`: fast unit coverage.
- `tests/system/`: CTest-driven system coverage.
- `docs/`: user, developer, and ADR documentation.
