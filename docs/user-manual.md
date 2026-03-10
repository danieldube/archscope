# User Manual

## Supported commands

`archscope --help`
: Print the currently supported command-line options.

`archscope --version`
: Print the bootstrap version string.

`archscope <compile_commands.json> <metrics...> --module=translation_unit [--report=<path>] [--project-name=<name>]`
: Load the compilation database, list translation units as modules, and write a
  Markdown report with computed metric values in the exact order requested on
  the command line.

Supported metric ids for this increment:

- `abstractness`
- `instability`
- `distance_from_main_sequence`

Example:

```bash
./build/archscope tests/fixtures/placeholder_project/compile_commands.json \
  instability abstractness --module=translation_unit \
  --report=architecture-metrics.md
```

The current translation-unit instability fixture produces:

- `src/alpha.cpp` with `Instability: 1.000`
- `src/beta.cpp` with `Instability: 0.000`

You can run that fixture directly from the repository root:

```bash
./build/archscope tests/fixtures/dependency_project/compile_commands.json \
  instability --module=translation_unit --report=/tmp/instability-report.md
```

## Implementation note

This increment requires Clang LibTooling development packages because the
analysis pipeline now parses translation units, enumerates class/struct
definitions, and extracts outgoing dependencies from base classes, fields, and
function signatures.

## Build and test

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
```
