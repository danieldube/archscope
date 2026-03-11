# ADR 0008: Use layered tests with committed fixtures for deterministic validation

- Status: Accepted

## Context

ArchScope combines pure metric logic, file-based compilation-database loading,
and Clang-powered source analysis. A single test style is not enough to cover
all of those concerns efficiently.

## Decision

Use a layered automated test strategy:

- Catch2 unit tests for pure core logic, error handling, formatting, and small
  Clang-adapter helpers
- CTest-driven system tests that execute the built `archscope` binary against
  committed fixture projects under `tests/fixtures/`
- Golden Markdown comparisons or robust parsing for end-to-end behavior,
  depending on the task

Fixtures must be small, deterministic, offline-runnable, and stored in the
repository with portable `compile_commands.json` content.

## Alternatives considered

- Rely only on system tests: rejected because failures would be slower to
  diagnose and would over-test unrelated layers.
- Mock Clang internals heavily in unit tests: rejected because it adds brittle
  test scaffolding and duplicates behavior already validated by system tests.
- Generate fixture projects dynamically during tests: rejected because it adds
  filesystem complexity and risks host-specific drift.

## Consequences

- Core computations can be developed test-first with fast feedback.
- End-to-end CLI behavior remains covered for each roadmap increment.
- Fixture maintenance becomes part of the architecture contract, not ad hoc
  test setup.
