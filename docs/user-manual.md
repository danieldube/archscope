# User Manual

## Supported commands

`archscope --help`
: Print the currently supported command-line options.

`archscope --version`
: Print the bootstrap version string.

`archscope <compile_commands.json> <metrics...> --module=<namespace|translation_unit|header> [--module-filter=<text>] [--report=<path>] [--project-name=<name>]`
: Load the compilation database, group results by translation unit, namespace,
  or header definition file, optionally filter the emitted module list, and
  write a Markdown report with computed metric values in the exact order
  requested on the command line.

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

Namespace filtering example:

```bash
./build/archscope tests/fixtures/namespace_project/compile_commands.json \
  abstractness instability --module=namespace --module-filter=a::b \
  --report=/tmp/namespace-report.md
```

Filtering rules in the current build:

- `--module=namespace`: prefix match, so `a::b` matches `a::b` and
  `a::b::detail`
- `--module=translation_unit`: substring match on the module path
- `--module=header`: substring match on the normalized header/source
  definition path

The current translation-unit distance fixture produces:

- `src/alpha.cpp` with `Abstractness: 0.667`, `Instability: 1.000`, and
  `Distance from the Main Sequence: 0.667`
- `src/beta.cpp` with `Abstractness: 1.000`, `Instability: 0.000`, and
  `Distance from the Main Sequence: 0.000`

You can run that fixture directly from the repository root:

```bash
./build/archscope tests/fixtures/dependency_project/compile_commands.json \
  abstractness instability distance_from_main_sequence \
  --module=translation_unit --report=/tmp/distance-report.md
```

You can also run the header filter fixture directly:

```bash
./build/archscope tests/fixtures/header_project/compile_commands.json \
  abstractness instability --module=header \
  --module-filter=include/domain/../domain/alpha.hpp \
  --report=/tmp/header-report.md
```

## Implementation note

This increment requires Clang LibTooling development packages because the
analysis pipeline parses translation units, enumerates class/struct
definitions, assigns translation-unit, namespace, and header ownership,
extracts outgoing dependencies from base classes, fields, and function
signatures, and combines those pure computations into `A`, `I`, and `D` for
each reported module.

## Build and test

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
```
