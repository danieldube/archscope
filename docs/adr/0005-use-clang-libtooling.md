# ADR 0005: Use Clang LibTooling for source analysis

- Status: Accepted

## Context

ArchScope must analyze real C++ projects through their compilation database,
respect the exact compiler arguments recorded there, and compute metrics from
semantic C++ information rather than text parsing.

The specification explicitly rules out a custom parser and requires the
production analysis path to work from Clang AST data.

## Decision

Use Clang LibTooling as the analysis frontend.

- Run analysis through `clang::tooling::ClangTool`.
- Build ASTs from the project’s `compile_commands.json`.
- Traverse declarations and types through Clang AST APIs, then translate the
  extracted data into the plain C++ domain model owned by `archscope_core`.

## Alternatives considered

- `libclang`: rejected because it offers a narrower cursor-oriented API and
  less direct control over the AST traversal needed for ownership and
  dependency extraction.
- Clang plugin: rejected because it complicates distribution and requires
  injecting project-specific compiler plugins into user builds.
- Custom parser: rejected because it would not be semantically correct for
  modern C++ and violates the specification.

## Consequences

- Analysis fidelity matches the recorded build configuration.
- Clang-specific dependencies stay isolated in `archscope_clang`.
- Native LLVM/Clang development packages are required for building ArchScope.
