# ADR 0004: Introduce a metric registry for CLI metric dispatch

- Status: Accepted

## Context

As ArchScope adds more metrics, dispatching each metric directly inside the CLI
would tightly couple argument handling, metric selection, and metric
implementation details.

The specification requires metrics to be reported in the order requested on the
CLI, and it expects new metrics to be added without reshaping the analysis
pipeline interfaces.

## Decision

Add `MetricRegistry` in `archscope_core` as the canonical mapping from
`MetricId` to pure metric computation functions.

- The CLI requests metric values through `MetricRegistry::compute`.
- The registry returns `MetricValue` entries in the exact order requested by the
  caller.
- Metric functions remain pure and depend only on `AnalysisResult` and
  `ModuleId`.

## Alternatives considered

- Keep metric dispatch in `src/cli/main.cpp`: rejected because it centralizes
  domain behavior in the executable and scales poorly as metrics grow.
- Introduce virtual metric classes immediately: rejected as unnecessary
  complexity at this stage while function-pointer dispatch is sufficient.

## Consequences

- The CLI remains thin and focused on argument parsing and IO.
- New metrics can be registered in one place and reused by tests.
- Ordering semantics become explicit and testable at the core layer.
