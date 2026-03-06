# User Manual

## Bootstrap commands

`archscope --help`
: Print the currently supported command-line options.

`archscope --version`
: Print the bootstrap version string.

## Build and test

```bash
./scripts/configure.sh
./scripts/build.sh
./scripts/test.sh
```

Later increments will extend this manual with compile database analysis, module
selection, and Markdown report generation. The current codebase already
includes internal support for reading `compile_commands.json`, but that
capability is not exposed on the CLI until the next roadmap tasks.
