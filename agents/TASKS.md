# TASKS.md — Implementation Roadmap (TDD + System-Tested Increments)

## Rules for every task
- Start by adding or updating **tests first** (unit + system as applicable).
- Each increment ends with:
  - `ctest` passing (unit + system)
  - `clang-format` clean
  - `clang-tidy` clean
  - CI workflow green locally (as close as possible)
  - Documentation updated (user manual / developer guide / ADRs)
- Each increment must ship a **fully runnable** `archscope` executable.

---

## Increment 0 — Repository skeleton + CI gates + “Hello CLI”

### Task 0.1 — Create repository scaffolding [completed]
1. Add CMake project skeleton with targets: - `archscope` executable -
   `archscope_core` library - `archscope_tests` (Catch2)
2. Add `.clang-format`, `.clang-tidy`, `.editorconfig`.
3. Add `pre-commit` configuration with clang-format and whitespace hooks.
4. Add GitHub Actions workflow with build + tests + format + tidy steps.

**Acceptance**
- `archscope --help` works.
- `ctest` runs and passes a trivial test.
- CI config present and locally runnable via scripts.

**System test**
- Add system test that runs `archscope --version` and checks output.

---

## Increment 1 — Compile database loading + basic report generation

### Task 1.1 — Tests first: compile_commands.json reader [completed]
- Unit tests for:
  - reading valid JSON file
  - handling missing file
  - handling invalid JSON
  - extracting a list of TU paths and compile args

### Task 1.2 — Implement compilation database loading [completed]
- Implement `CompilationDatabaseLoader` that returns entries:
  - source path
  - compile args (split safely)
  - working directory

Prefer Clang tooling `CompilationDatabase` APIs where possible; otherwise parse
JSON.

### Task 1.3 — Markdown report writer [completed]
- Unit tests for deterministic formatting and ordering.
- Implement minimal report writer producing the required header + module
  blocks.

**Acceptance**
- CLI supports: ```bash archscope <compile_db> abstractness
  --module=translation_unit --report=out.md ``` but values can be placeholders
  (`0.000`) for now.

**System test**
- Add a small fixture project with a generated `compile_commands.json`.
- System test runs `archscope` and compares report to expected placeholder
  output.

---

## Increment 2 — Clang LibTooling pipeline + enumerate types per TU

### Task 2.1 — Tests first: “types extracted” domain model [completed]
- Unit tests for `AnalysisResult` assembly from synthetic extracted data.

### Task 2.2 — Implement Clang tool runner (syntax-only) [completed]
- Integrate `clang::tooling::ClangTool` and a minimal `FrontendAction` that
  parses TU.
- Collect:
  - counted types (class/struct definitions only)
  - their source locations (file path)
  - their qualified names

No metric computation yet; just extraction.

### Task 2.3 — Map extracted types to translation unit module
- Owner = TU source file path.

**Acceptance**
- `--module=translation_unit` produces module list and counts.
- Metrics still placeholder.

**System test**
- Fixture contains at least two `.cpp` files with classes.
- System test ensures both modules appear.

---

## Increment 3 — Abstractness (A) for translation_unit modules

### Task 3.1 — Tests first: abstractness metric [completed]
- Unit tests:
  - empty module -> A=0
  - 1 abstract, 1 concrete -> A=0.5
  - only abstract -> A=1
  - only concrete -> A=0

### Task 3.2 — Implement “abstract vs concrete” classification [completed]
- Use `CXXRecordDecl::isAbstract()` for definitions.
- Exclude forward decls and system headers.

### Task 3.3 — Wire metric registry + CLI selection [completed]
- Implement `MetricRegistry`.
- Ensure requested metrics determine output order.

**Acceptance**
- CLI computes correct abstractness per TU.

**System test**
- Fixture updated to include an abstract class (pure virtual).
- Golden Markdown matches expected A values.

---

## Increment 4 — Dependency extraction (outgoing) for translation_unit modules

### Task 4.1 — Tests first: dependency graph builder [completed]
- Unit tests for:
  - edge deduplication
  - ignoring self-dependencies
  - ignoring system headers

### Task 4.2 — Implement dependency extraction in visitor [completed]
Track referenced types via:
- base classes
- fields
- return/param types

Resolve referenced decl location → owner module.

### Task 4.3 — Compute Ce / Ca / I [completed]
- Implement instability metric with corner case Ce+Ca=0 -> I=0.

**Acceptance**
- CLI computes instability per TU.

**System test**
- Fixture with TU1 using a type defined in TU2.
- Expected: TU1 Ce>=1, TU2 Ca>=1; verify exact values for the fixture.

---

## Increment 5 — Distance from Main Sequence (D)

### Task 5.1 — Tests first: distance metric [completed]
- Unit tests cover:
  - D=|A+I-1| with clamping
  - exact known cases

### Task 5.2 — Implement distance metric and integrate [completed]
- Ensure formatting stable.

**Acceptance**
- CLI computes A, I, D per TU.

**System test**
- Golden output verifies all three values.

---

## Increment 6 — Namespace modules (ownership + filtering)

### Task 6.1 — Tests first: namespace module resolver [completed]
- Unit tests for:
  - global namespace
  - nested namespaces
  - anonymous namespaces

### Task 6.2 — Implement namespace ownership [completed]
- Owner = fully qualified namespace of the type definition.

### Task 6.3 — Implement `--module-filter` prefix behavior [completed]
- Filter module output only (analysis still processes all).

**Acceptance**
- `--module=namespace` works and produces correct groups.

**System test**
- Fixture with `a::b` and `a::c`; filter `a::b` yields only that module.

---

## Increment 7 — Header modules (ownership + filtering)

### Task 7.1 — Tests first: header ownership rules
- Unit tests for mapping a definition location to a header module.
- Include a header-only class and a `.cpp`-defined class.

### Task 7.2 — Implement header ownership
- Owner = file of spelling location for the definition.
- Ignore system headers.

### Task 7.3 — Module-kind specific filtering
- Header filter is substring match on normalized path.

**Acceptance**
- `--module=header` works on fixture.

**System test**
- Fixture uses headers across modules; golden output validates.

---

## Increment 8 — Parallel execution + determinism

### Task 8.1 — Tests first: determinism under parallelism
- System test runs `archscope` twice with `--threads=1` and `--threads=4`;
  outputs must match.

### Task 8.2 — Implement thread pool execution
- Parallelize per-TU processing.
- Aggregate results with deterministic ordering.

**Acceptance**
- Speed-up possible; output deterministic.

**System test**
- Determinism test passes.

---

## Increment 9 — Documentation + ADR completion

### Task 9.1 — User manual
- Add full CLI docs, examples for all module kinds, troubleshooting.

### Task 9.2 — Developer guide
- Adding a new metric walkthrough.
- Adding a new dependency extractor rule.

### Task 9.3 — ADR set
Required ADRs (minimum):
- ADR: Use Clang LibTooling (vs plugin/libclang)
- ADR: Module ownership rules (namespace/TU/header)
- ADR: Dependency definition and exclusions
- ADR: Test strategy (unit + system fixtures)

**Acceptance**
- Docs complete and internally consistent.
- CI gates still pass.

**System test**
- No new system test required, but existing must pass.

---

## Increment 10 — Hardening: diagnostics, errors, and UX polish

### Task 10.1 — Tests first: error cases
- System tests for:
  - missing compile db file (exit code 3)
  - clang parse failure (exit code 4)
  - invalid module kind (exit code 2)

### Task 10.2 — Implement robust error reporting
- Structured errors with context.
- `--verbose` logs.

**Acceptance**
- Clear errors and correct exit codes.

**System test**
- All error-case system tests pass.

---

## Definition of “Ready for Review” (applies to every increment)
- All tests pass.
- All linters pass.
- Output stable and documented.
- ADR updated if any non-trivial decision was made.
- No TODOs for shipped behavior (allowed only for future work, clearly marked
  and tracked).
