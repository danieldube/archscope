# User Manual

## Supported commands

`archscope --help`
: Print the currently supported command-line options.

`archscope --version`
: Print the bootstrap version string.

`archscope <compile_commands.json> <metrics...> --module=translation_unit [--report=<path>] [--project-name=<name>]`
: Load the compilation database, list translation units as modules, and write a
  Markdown report with placeholder metric values for the requested metrics.

Supported metric ids for this increment:

- `abstractness`
- `instability`
- `distance_from_main_sequence`

Example:

```bash
./build/archscope tests/fixtures/placeholder_project/compile_commands.json \
  abstractness --module=translation_unit --report=architecture-metrics.md
```

## Implementation note

This increment keeps the user-visible report output unchanged, but the build now
requires Clang LibTooling development packages because the internal analysis
pipeline can parse translation units and enumerate class/struct definitions.

## Build and test

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
```
