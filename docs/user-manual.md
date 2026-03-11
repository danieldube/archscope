# User Manual

## Overview

`archscope` analyzes a project through its `compile_commands.json`, groups the
discovered types into modules, computes Robert C. Martin package metrics, and
writes a deterministic Markdown report.

## Command reference

`archscope --help`
: Print usage, supported metrics, and supported options.

`archscope --version`
: Print the executable version string.

`archscope <compile_commands.json> <metrics...> --module=<namespace|translation_unit|header|compilation_target> [--module-filter=<text>] [--report=<path>] [--project-name=<name>] [--threads=<n>] [--verbose]`
: Load the compilation database, group results by translation unit, namespace,
  header definition file, or compilation target, optionally filter the emitted
  module list, and write a Markdown report with metric values in the exact
  order requested on the command line.

## Metrics

- `abstractness`
- `instability`
- `abstract_type_count`
- `concrete_type_count`
- `type_count`
- `distance_from_main_sequence`

Metrics are emitted in the same order they are requested on the CLI.

Metric notes:

- `instability` for `--module=compilation_target` uses compilation-target
  membership for header-defined types. If two targets both compile the same
  header-defined API type, ArchScope can emit couplings between those targets
  even though the header has no direct `compile_commands.json` entry.

## Options

- `--module=<kind>`: required; one of `translation_unit`, `namespace`, or
  `header`, or `compilation_target`
- `--module-filter=<text>`: optional output filter; analysis still runs over
  the full project graph
- `--report=<path>`: output Markdown file path; defaults to
  `architecture-metrics.md`
- `--project-name=<name>`: optional report header override
- `--threads=<n>`: optional parallelism request; values are clamped into the
  valid runtime range
- `--verbose`: emit progress logs and extended error context on stderr
- `--help`: print usage and exit
- `--version`: print version and exit

## Module kinds and filtering

- `translation_unit`: module owner is the translation-unit source file path;
  `--module-filter` uses substring matching on that path
- `namespace`: module owner is the fully qualified namespace; global scope is
  `<global>`, anonymous namespaces are emitted as `<anonymous>` within their
  parent scope; `--module-filter` uses prefix matching
- `header`: module owner is the normalized spelling path of the definition
  file; `--module-filter` uses substring matching on the normalized path
- `compilation_target`: module owner is the compilation target derived from the
  compile command output metadata; CMake-style
  `CMakeFiles/<target>.dir/...` outputs emit `<target>`, otherwise ArchScope
  falls back to the normalized object-output directory or source path;
  header-defined types can belong to multiple compilation targets when multiple
  targets compile them, and `--module-filter` uses exact matching

## Examples

Translation-unit report:

```bash
./build/archscope tests/fixtures/placeholder_project/compile_commands.json \
  instability abstractness --module=translation_unit \
  --report=architecture-metrics.md
```

Namespace report with filtering:

```bash
./build/archscope tests/fixtures/namespace_project/compile_commands.json \
  abstractness instability --module=namespace --module-filter=a::b \
  --report=/tmp/namespace-report.md
```

Header report with normalized-path filtering:

```bash
./build/archscope tests/fixtures/header_project/compile_commands.json \
  abstractness instability --module=header \
  --module-filter=include/domain/../domain/alpha.hpp \
  --report=/tmp/header-report.md
```

Compilation-target report:

```bash
./build/archscope tests/fixtures/compilation_target_project/compile_commands.json \
  abstractness instability abstract_type_count concrete_type_count \
  type_count distance_from_main_sequence \
  --module=compilation_target --report=/tmp/target-report.md
```

That fixture currently produces:

- `demo_app` with `Abstractness: 0.333`, `Instability: 0.500`,
  `Abstract Types: 1`, `Concrete Types: 2`, `Types: 3`, and
  `Distance from the Main Sequence: 0.167`
- `demo_domain` with `Abstractness: 0.500`, `Instability: 0.500`,
  `Abstract Types: 1`, `Concrete Types: 1`, `Types: 2`, and
  `Distance from the Main Sequence: 0.000`

Translation-unit report with explicit thread count:

```bash
./build/archscope tests/fixtures/parallel_project/compile_commands.json \
  abstractness instability abstract_type_count concrete_type_count \
  type_count distance_from_main_sequence \
  --module=translation_unit --threads=4 \
  --report=/tmp/parallel-report.md
```

## Report format

The generated Markdown always uses stable ordering:

- modules sorted lexicographically by their emitted id
- metrics listed in CLI request order
- numbers formatted with three decimal places

Minimum report structure:

```md
**project-name**

module-id:
 * Abstract Types: 2
 * Concrete Types: 1
 * Types: 3
 * Abstractness: 0.500
 * Instability: 0.250
 * Distance from the Main Sequence: 0.250
```

## Fixture-backed examples

The current translation-unit distance fixture produces:

- `src/alpha.cpp` with `Abstractness: 0.667`, `Instability: 1.000`, and
  `Abstract Types: 2`, `Concrete Types: 1`, `Types: 3`, and
  `Distance from the Main Sequence: 0.667`
- `src/beta.cpp` with `Abstractness: 1.000`, `Instability: 0.000`,
  `Abstract Types: 1`, `Concrete Types: 0`, `Types: 1`, and
  `Distance from the Main Sequence: 0.000`

You can run that fixture directly from the repository root:

```bash
./build/archscope tests/fixtures/dependency_project/compile_commands.json \
  abstractness instability abstract_type_count concrete_type_count \
  type_count distance_from_main_sequence \
  --module=translation_unit --report=/tmp/distance-report.md
```

The parallel determinism fixture compares repeated `--threads=1` and
`--threads=4` runs byte-for-byte for translation-unit, namespace, header, and
compilation-target reports.

## Troubleshooting

Structured errors are emitted on stderr in this form:

```text
error: <category>
  message: <message>
  <context-label>: <context-value>
```

Use `--verbose` to add progress logs before a failure, or to confirm which
analysis stage the command is currently executing.

- `error: usage error` with `option: --module=<kind>`
  Provide `--module=translation_unit`, `--module=namespace`,
  `--module=header`, or `--module=compilation_target`.
- `error: usage error` with `metric: <name>`
  Use one or more of `abstractness`, `instability`, `abstract_type_count`,
  `concrete_type_count`, `type_count`, and
  `distance_from_main_sequence`.
- Exit code `3`
  The compilation database could not be read or parsed. Verify the path points
  to a valid `compile_commands.json` and that relative `directory` values are
  correct relative to the database file.
- Exit code `4`
  Clang failed to parse at least one translation unit. Rebuild the target
  project’s compile database and confirm the listed source file still compiles
  with the recorded arguments.
- Exit code `5`
  ArchScope hit an internal failure while writing the report or handling an
  unexpected exception. Check the structured `report:` context line first to
  confirm the output path is writable and that none of its parent components is
  an existing regular file.
- No modules in the report
  Check whether `--module-filter` excluded every emitted module. Filtering is
  applied after analysis, so the filter only affects report output.
- Unexpectedly low `instability` for `--module=compilation_target`
  ArchScope can only emit cross-target couplings for types whose definition
  paths can be associated with one or more compilation targets. If a target
  does not compile a referenced definition path anywhere in the database, that
  dependency cannot contribute to `Ce`/`Ca`.
- LLVM/Clang packages not found during configure
  Install the development packages listed in `README.md` and rerun CMake, or
  pass `-DARCHSCOPE_LLVM_VERSION=<version>` to point CMake at the intended
  installation.

## Build and test

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
```
