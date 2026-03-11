# ArchScope

ArchScope is a C++17 command-line tool that computes architecture metrics from
projects with a `compile_commands.json` database. It uses Clang LibTooling for
semantic C++ analysis and emits deterministic Markdown reports for
translation-unit, namespace, header, and compilation-target modules.

## Installation

Install the required native toolchain first. On Ubuntu 24.04:

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

If CMake cannot locate LLVM/Clang automatically, configure with
`-DARCHSCOPE_LLVM_VERSION=18`.

## Quick start

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
pre-commit run --all-files
./build/archscope --help
./build/archscope --version
```

Run a translation-unit report:

```bash
./build/archscope tests/fixtures/placeholder_project/compile_commands.json \
  abstractness --module=translation_unit --report=/tmp/archscope-report.md
```

Run a namespace report with prefix filtering:

```bash
./build/archscope tests/fixtures/namespace_project/compile_commands.json \
  abstractness instability --module=namespace --module-filter=a::b \
  --report=/tmp/archscope-namespace-report.md
```

Run a deterministic multi-metric report in parallel:

```bash
./build/archscope tests/fixtures/parallel_project/compile_commands.json \
  abstractness instability abstract_type_count concrete_type_count \
  type_count distance_from_main_sequence \
  --module=translation_unit --threads=4 \
  --report=/tmp/archscope-deterministic-report.md
```

Run a compilation-target report grouped by build target:

```bash
./build/archscope tests/fixtures/compilation_target_project/compile_commands.json \
  abstractness instability --module=compilation_target \
  --report=/tmp/archscope-target-report.md
```

For that fixture, the refined compilation-target coupling model reports
`Instability: 0.500` for both `demo_app` and `demo_domain`, because both
targets compile the shared header-defined `SharedPort` type and therefore
depend on each other through that shared membership.

## CLI summary

Canonical form:

```bash
archscope <compile_commands.json> <metrics...> --module=<kind> \
  [--module-filter=<text>] [--report=<path>] [--project-name=<name>] \
  [--threads=<n>] [--verbose]
```

Supported metrics:

- `abstractness`
- `instability`
- `abstract_type_count`
- `concrete_type_count`
- `type_count`
- `distance_from_main_sequence`

Supported module kinds:

- `translation_unit`
- `namespace`
- `header`
- `compilation_target`

Filter behavior:

- `translation_unit`: substring match on the emitted source path
- `namespace`: prefix match on the fully qualified namespace
- `header`: substring match on the normalized definition path
- `compilation_target`: exact match on the emitted target id

Compilation-target coupling behavior:

- target ids come from compile-command output metadata
- header-defined types can belong to multiple compilation targets
- `instability` for `compilation_target` uses that shared membership to recover
  cross-target couplings even when the header itself has no compile-db entry

Exit codes:

- `0`: success
- `2`: usage error
- `3`: compile database error
- `4`: analysis error
- `5`: internal error

Error output is structured on stderr with a category, message, and contextual
fields such as the failing path or translation unit. `--verbose` adds progress
logs for database loading, analysis, projection, and report writing.

## Current status

The CLI accepts a compilation database path, one or more metric ids, and a
required module kind, then writes a deterministic Markdown report.
`abstractness`, `instability`, `abstract_type_count`,
`concrete_type_count`, `type_count`, and
`distance_from_main_sequence` are wired through the metric registry and
emitted in requested CLI order.
`archscope_clang` parses translation units with Clang LibTooling, enumerates
user-defined class/struct definitions with qualified names, definition paths,
namespace owners, and abstract/concrete classification, and extracts
dependency candidates from base classes, fields, and function signatures.
`--module-filter` filters only the emitted module list, not the underlying
analysis graph. `--threads=<n>` runs per-translation-unit extraction in
parallel while preserving deterministic output ordering. Compilation-target
ownership is derived from each compile command's object output metadata,
preferring CMake-style `CMakeFiles/<target>.dir/...` target names when
available. Header-defined types can belong to multiple compilation targets when
multiple targets compile them, and compilation-target instability uses that
membership to recover cross-target couplings for shared headers.

## Documentation map

- `docs/user-manual.md`: full CLI reference, examples, and troubleshooting
- `docs/developer-guide.md`: architecture, build/test workflow, and extension
  walkthroughs
- `docs/adr/`: architecture decisions and rationale

## Repository layout

- `src/cli/`: executable entrypoint.
- `src/core/`: reusable logic shared by the executable and tests, including
  compilation-database loading, metric computation, filtering, and report
  rendering.
- `src/clang/`: Clang LibTooling integration that extracts plain C++ type data.
- `src/report/`: reserved for report generation.
- `tests/unit/`: fast unit coverage.
- `tests/system/`: CTest-driven system coverage.
- `docs/`: user, developer, and ADR documentation.
