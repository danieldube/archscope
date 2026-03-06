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

## Next implementation steps

The next increments will replace bootstrap placeholders with compilation
database loading, Clang LibTooling integration, and metric/report generation
while preserving the current target layout.
