# ADR 0003: Add a dedicated Clang LibTooling adapter library

- Status: Accepted

## Context

The roadmap now requires real Clang AST parsing, but `archscope_core` must stay
free of Clang headers and AST-specific concepts. The build also needs a
repeatable way to discover LibTooling from the locally installed LLVM packages.

## Decision

Add a separate `archscope_clang` static library that owns:

- adaptation from the core compilation-database model to
  `clang::tooling::CompilationDatabase`
- syntax-only `ClangTool` execution
- AST traversal for extracting plain C++ type records

Resolve Clang tooling through CMake `find_package(LLVM CONFIG)` and
`find_package(Clang CONFIG)`, using `ARCHSCOPE_LLVM_VERSION` as the primary
hint for the expected `/usr/lib/llvm-<version>` install layout.

## Alternatives considered

- Put Clang traversal directly into `archscope_core`: rejected because it would
  violate the core/Clang boundary in the specification.
- Re-parse `compile_commands.json` inside the Clang layer with Clang's JSON
  loader: rejected because the repository already has a tested loader and
  duplicating that logic would create drift.
- Delay the dedicated library until metric computation: rejected because the
  target boundary is part of the architecture, not just a later refactor.

## Consequences

- Clang-specific headers and link dependencies stay isolated to
  `archscope_clang`.
- The current CLI target can link the future analysis pipeline incrementally
  without changing the core domain model boundary.
- Builds now depend on an installed LLVM/Clang development package set with
  LibTooling CMake config files.
