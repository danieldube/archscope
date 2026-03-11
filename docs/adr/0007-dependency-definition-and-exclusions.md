# ADR 0007: Define dependency edges from user-type references and explicit exclusions

- Status: Accepted

## Context

Instability depends on the outgoing and incoming module graph. Without a tight
definition of what creates an edge, the reported values would drift as the
implementation evolves.

## Decision

For the MVP, create a module dependency edge `M1 -> M2` when analysis of an
entity owned by `M1` references a user-defined type owned by `M2` through one
of these AST constructs:

- base classes
- field member types
- function return types
- function parameter types
- template type arguments reachable through those types

Apply these ownership rules:

- Class-level metadata is recorded for each extracted dependency (`from_qualified_type` and `target_qualified_type`).
- Function-signature dependencies are collected only for class/struct methods; free-function dependencies are excluded so class ownership stays explicit and deterministic.

Apply these exclusions:

- ignore self-dependencies
- ignore duplicate edges
- ignore built-in types
- ignore declarations from system headers
- ignore invalid macro-only locations
- ignore `using` declarations and directives for dependency purposes

## Alternatives considered

- Count every symbol reference, including aliases and namespace imports:
  rejected because it produces noisy graphs and unstable `Ce`/`Ca` values.
- Count repeated references multiplicatively: rejected because the metric uses
  set-based couplings, not frequency.
- Include standard library and system-header types: rejected because the MVP
  focuses on project architecture, not toolchain internals.

## Consequences

- `Ce` and `Ca` remain deterministic and explainable.
- System tests can assert exact graph-derived metric values.
- Future extractor expansions must be documented when they add a new edge kind.
