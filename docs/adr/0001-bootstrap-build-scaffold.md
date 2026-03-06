# ADR 0001: Bootstrap the project with a self-contained CMake test scaffold

- Status: Accepted

## Context

The roadmap starts with repository scaffolding, a runnable CLI, unit tests, and
a system test. The current environment is offline, so external package
downloads cannot be assumed during the bootstrap increment.

## Decision

Use a self-contained CMake/Ninja scaffold with these targets:

- `archscope` executable
- `archscope_core` library
- `archscope_tests` test executable

Keep the CLI implementation standard-library only for the bootstrap step, and
add a local `catch2`-compatible test shim so the first increment stays
buildable without network access.

## Alternatives considered

- Depend on system-installed Catch2 and CLI11: rejected because those packages
  are not available in this environment.
- Fetch dependencies during configure: rejected because the bootstrap increment
  must not rely on network access.

## Consequences

- The repository is immediately buildable and testable offline.
- Later increments can replace the temporary test shim and bootstrap parser
  with packaged dependencies without changing the target layout.
