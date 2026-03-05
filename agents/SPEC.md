# AGENTS.md — Specification for ArchScope (C++17, Clang LibTooling)

## 1. Purpose

Build a professional, maintainable C++17 CLI tool named `ArchScope` with the executable `archscope` that computes Robert C. Martin package metrics over a C++ code base using a `compile_commands.json` compilation database.

Initial metrics (per module):
- **Abstractness (A)**: `A = Na / (Na + Nc)`
  - `Na`: number of **abstract** types in module
  - `Nc`: number of **concrete** types in module
- **Instability (I)**: `I = Ce / (Ce + Ca)`
  - `Ce`: efferent couplings (outgoing dependencies from module)
  - `Ca`: afferent couplings (incoming dependencies into module)
- **Distance from Main Sequence (D)**: `D = |A + I - 1|`

ArchScope must:
- Run on **any** C++ project that provides a valid `compile_commands.json`.
- Use Clang's AST and semantic analysis (no custom parser).
- Generate a **Markdown** report.

This repository is designed to be developed **entirely by a generative AI agent** using the processes and constraints described here.

---

## 2. Scope and Non-Goals

### In scope (MVP)
- Read and validate a compilation database (`compile_commands.json`).
- Run Clang tooling over the compilation database to build ASTs.
- Support module grouping by:
  1. **Namespace module** (e.g., `--module=namespace --module-filter=my::ns`)
  2. **Translation unit module** (each source file entry)
  3. **Header module** (headers as owners of declarations; see §8.3)
- Compute A, I, D for each module.
- Write a Markdown report to file.

### Explicit non-goals (MVP)
- No GUI.
- No graph export formats (JSON, CSV) unless added later.
- No incremental caching in the MVP.
- No cross-language support beyond C/C++ compiled by Clang front-end.

---

## 3. User Interface

### Command line

Canonical invocation:
```bash
archscope path/to/compile_commands.json abstractness instability distance_from_main_sequence --module=namespace --module-filter=my::ns
```

#### Positional arguments
1. `compile_commands_path`: path to `compile_commands.json`
2. `metrics...`: one or more metric identifiers from:
   - `abstractness`
   - `instability`
   - `distance_from_main_sequence`

#### Options
- `--module=<kind>` where `<kind>` is one of:
  - `namespace`
  - `translation_unit`
  - `header`
- `--module-filter=<string>`
  - If `--module=namespace`: filter module names by **prefix match** (e.g., `my::ns` matches `my::ns`, `my::ns::detail`).
  - If `--module=translation_unit`: filter by file path substring.
  - If `--module=header`: filter by header path substring.
- `--report=<path>` output Markdown file (default: `architecture-metrics.md` in current working directory)
- `--project-name=<string>` optional override for report header (default: derived from parent directory of `compile_commands.json`)
- `--threads=<n>` optional parallelism (default: `std::thread::hardware_concurrency()`; must be clamped to [1..N])
- `--verbose` increase logging verbosity
- `--version`
- `--help`

### Exit codes
- `0`: success
- `2`: CLI usage error (invalid args/options)
- `3`: compilation database error (cannot read/parse or invalid entries)
- `4`: analysis error (Clang failed to parse one or more translation units)
- `5`: internal error (unexpected exceptions)

### Output report format (Markdown)

Minimum structure:
```md
**<Project Name>**

<Module Name>:
 * Abstractness: <number>
 * Instability: <number>
 * Distance from the Main Sequence: <number>

...
```

Notes:
- Use **stable ordering**:
  - Modules sorted lexicographically by name/path.
  - Metrics listed in the order requested on CLI.
- Format numbers with:
  - 3 decimals by default (e.g., `0.500`), unless exact `0` or `1` occurs; then still `0.000` / `1.000` for consistency.

---

## 4. Definitions and Computation Rules

### 4.1 What counts as a “type”
A “type” is any of the following Clang declarations found in the translation unit:
- `CXXRecordDecl` (class/struct)
- `ClassTemplateDecl` (counts as a type; abstractness determined from the templated record)
- `ClassTemplateSpecializationDecl` does **not** count separately if it is an instantiation of a counted template (avoid double counting).
- `EnumDecl` is **excluded** from A (treated neither abstract nor concrete).
- `ConceptDecl` excluded.

### 4.2 Abstractness (A)
A type is **abstract** if:
- It is a C++ class/struct with **at least one pure virtual function**; i.e., `CXXRecordDecl::isAbstract()` is true.
- OR it is a class declared as `=0` pure virtual method in any base class that makes it abstract (Clang will reflect this in `isAbstract()`).

A type is **concrete** if:
- It is a complete definition (has a definition) and is not abstract.

Exclusions:
- Forward declarations without definitions are ignored.
- Dependent or invalid declarations are ignored.

Module-level A:
- `A = Na / (Na + Nc)`
- If `(Na + Nc) == 0`, define `A = 0.0` (empty module is treated concrete).

### 4.3 Couplings (Ce, Ca) and Instability (I)

We define a **module dependency** as: module `M1` depends on module `M2` if within the AST of entities owned by `M1`, there exists at least one **referenced type** or **symbol** that is declared/defined in `M2`.

For the MVP, dependencies are computed from **type references**:
- Base classes (`CXXBaseSpecifier`)
- Field member types (`FieldDecl::getType()`)
- Function return types and parameter types (`FunctionDecl`)
- Using declarations (`UsingDecl`, `UsingDirectiveDecl`) are ignored for dependency purposes (too noisy).
- Template arguments: include type arguments when they refer to a user type.

We compute dependencies as **set-based** (no multiplicity):
- `Dep(M1) = { M2 | M1 depends on M2 and M2 != M1 }`
- `Ce(M1) = |Dep(M1)|`
- `Ca(M1) = |{ M2 | M2 depends on M1 and M2 != M1 }|`

Instability:
- If `(Ce + Ca) == 0`, define `I = 0.0`.

### 4.4 Distance from main sequence (D)
- `D = abs(A + I - 1)`
- Must be clamped to [0, 1] due to floating arithmetic.

---

## 5. Architecture

### 5.1 High-level components
- `archscope` (executable): parses args, orchestrates analysis, writes report.
- `archscope-core` (library): domain model, metric computation, module grouping, report generation logic.
- `archscope-clang` (library): Clang LibTooling integration, AST visitors, symbol extraction.
- `archscope-tests` (test targets): unit tests + system tests.

### 5.2 Clean architecture boundaries
- `archscope-core` must not include Clang headers.
- `archscope-clang` translates Clang AST into a **plain C++ domain model**:
  - Types, ownership (module id), and dependency edges.

### 5.3 Domain model (core)
Minimum types:
- `ModuleId` (strong type wrapper around `std::string`)
- `ModuleKind` enum
- `TypeId` (string)
- `TypeInfo`:
  - `TypeId id`
  - `ModuleId owner`
  - `bool is_abstract`
  - `bool is_concrete`
- `DependencyGraph`:
  - `std::unordered_map<ModuleId, std::unordered_set<ModuleId>> outgoing`
  - derived `incoming` computed lazily or built explicitly
- `AnalysisResult`:
  - `std::vector<ModuleId> modules`
  - `std::vector<TypeInfo> types`
  - `DependencyGraph graph`

### 5.4 Metrics subsystem
- `MetricId` enum/string
- `Metric` interface:
  - `MetricId id() const`
  - `std::string display_name() const`
  - `double compute(const AnalysisResult&, const ModuleId&) const`
- `MetricRegistry` mapping id → factory.

Metrics must be pure functions:
- No filesystem, no I/O, no global state.

### 5.5 Report subsystem
- `ReportWriter`:
  - `std::string to_markdown(const ReportModel&)`
  - `void write_file(path, content)`
- `ReportModel`:
  - project name
  - ordered list of modules with metric values

### 5.6 Error handling policy
- No exceptions crossing CLI boundary (catch at `main`).
- Core functions return `Expected<T, Error>` (use `tl::expected` or equivalent).
- Errors include:
  - code, message, context.

---

## 6. Build, Toolchain, and Dependencies

### 6.1 Toolchain requirements
- C++17 compiler: Clang >= 16 or GCC >= 12 for building `ArchScope` itself.
- LLVM/Clang development packages matching a supported version of Clang tooling libraries.
- CMake >= 3.26 recommended.
- Ninja recommended.

### 6.2 Required third-party dependencies (MVP)
- **LLVM/Clang**: LibTooling, clang-cpp, LLVMSupport
- **CLI11**: argument parsing
- **fmt**: formatting (or use `std::format` if toolchain supports reliably across platforms)
- **spdlog**: logging (optional in MVP; can be introduced after baseline)
- **nlohmann/json**: parse compilation database (optional; LLVM JSON is acceptable)
- **Catch2 v3**: unit tests

Dependency management:
- Prefer **vcpkg manifest mode** (`vcpkg.json`) OR **Conan 2**. Choose one and standardize. (If already selected by project, follow existing.)
- Clang/LLVM is typically system-provided; document how to point CMake to it.

### 6.3 Project layout (mandatory)
```
/CMakeLists.txt
/cmake/                # custom cmake modules
/src/
  /cli/
  /core/
  /clang/
  /report/
/tests/
  /unit/
  /system/
/docs/
  /adr/
/scripts/
/.third_party/ (only if needed; prefer package manager)
/.clang-format
/.clang-tidy
/.editorconfig
/.pre-commit-config.yaml
/.github/workflows/ci.yml
```

---

## 7. Quality Requirements

### 7.1 Coding standard
- C++17, no compiler extensions unless guarded.
- Use the LLVM coding style as the baseline formatting convention.
- Enforce formatting via `clang-format` configured for LLVM style (with project-specific overrides only if explicitly documented).
- Prefer value semantics; explicit ownership.
- No raw `new/delete`.
- Minimize macros.
- Use `std::span`, `std::optional`, `std::variant`, `std::filesystem`.
- Use `gsl::not_null` optionally if available.

### 7.2 Static analysis
Must pass in CI:
- `clang-format` check (no diffs)
- `clang-tidy` with a curated configuration
- `cmake --build` warnings treated as errors (where practical)
- `ctest` all tests pass
- Optional: `include-what-you-use` can be added later

### 7.3 Testing
- Use Catch2 for unit tests.
- Coverage target: “high coverage of core logic”; do not test Clang internals.
- System tests are mandatory for each increment:
  - Execute built `archscope` against a **small fixture project** with its own `compile_commands.json`.
  - Validate output Markdown content exactly (golden file) or by robust parsing.

All tests follow F.I.R.S.T. principles:
- Fast, Independent, Repeatable, Self-validating, Timely.

### 7.4 Development method
**Test-driven development (TDD) is mandatory**:
- For every behavior change, first add/modify a test that fails.
- Then implement production code to make it pass.
- Then refactor.

Every increment in the roadmap must:
- Produce a runnable CLI.
- Add/extend system test coverage.

### 7.5 Documentation
Must include:
- `README.md`: overview, install, quick start
- `docs/user-manual.md`: full CLI usage, examples, troubleshooting
- `docs/developer-guide.md`: architecture, build, test, adding metrics
- `docs/adr/`: Architecture Decision Records
  - Every non-trivial decision must have an ADR (see template).

ADR template (`docs/adr/NNNN-title.md`):
- Context
- Decision
- Alternatives considered
- Consequences
- Status (Accepted/Superseded)

---

## 8. Clang Tooling Integration Details

### 8.1 Using compilation database
- Load compilation database with `clang::tooling::CompilationDatabase::loadFromDirectory` or JSON parsing as needed.
- Ensure support for compile databases generated by CMake and tools like `bear`.

### 8.2 AST traversal approach
- Use `clang::tooling::ClangTool` to run a `FrontendAction`.
- Inside `ASTConsumer`, walk the translation unit with:
  - `RecursiveASTVisitor` OR AST Matchers.
- For MVP, start with `RecursiveASTVisitor` for explicit control and easier ownership assignment.

### 8.3 Module ownership rules
Ownership determines where types and dependencies “belong”.

#### Namespace module
- Owner module id = the **fully qualified namespace name** containing the declaration.
- Anonymous namespace: treat as `"<anonymous>"` and include parent scope.
- Types in global namespace: module id = `"<global>"`.

#### Translation unit module
- Owner module id = normalized absolute (or project-relative) path of the TU source file.

#### Header module (MVP definition)
- Owner module id = normalized header path of the file where the type is **defined** (`SourceManager` spelling location).
- If definition is in a `.cpp`, it becomes its own owner by that file path (still valid).
- For system headers: ignore types defined in system headers (unless configured later).

### 8.4 Dependency extraction for MVP
When visiting a type owned by module M:
- For each relevant referenced type symbol Tref:
  - Resolve its declaration location → determine owner module M2.
  - If M2 differs from M, add edge M → M2.

Ignore dependencies to:
- Built-in types
- Standard library/system headers (detected via `SourceManager::isInSystemHeader`)
- Macros expansions when location is invalid

---

## 9. System Test Fixture Projects

A system test fixture must be small, deterministic, and compilable.

Structure example:
```
tests/system/fixtures/simple/
  compile_commands.json
  include/
  src/
  CMakeLists.txt (optional, but fixture should include generated compile db)
```

The fixture should include:
- At least 3 modules (namespaces or files) with known dependencies
- Abstract and concrete classes to validate A
- Dependency edges to validate Ce/Ca and I, and thus D

System tests run via CTest:
- Build `archscope`
- Run fixture analysis
- Compare report output to expected content

---

## 10. CI and Pre-commit

### 10.1 Pre-commit hooks
Use `pre-commit` with:
- clang-format
- basic whitespace checks
- cmake-format (optional)
- markdownlint (optional)

### 10.2 GitHub Actions CI (mandatory gates)
Workflow stages:
1. Configure + build (Debug and Release)
2. Run unit tests
3. Run system tests
4. clang-format check
5. clang-tidy check

CI must fail on any gate failure.

---

## 11. Security, Reliability, and Determinism

- `ArchScope` must not execute user-provided commands.
- Do not interpret compile_commands command strings; use them only as compiler invocation arguments.
- Avoid non-deterministic ordering (sort outputs).
- Ensure paths are normalized consistently.

---

## 12. Acceptance Criteria (MVP)

The project is accepted when:
- `archscope` builds on Linux and macOS (Windows optional if desired later).
- Running:
  ```bash
  archscope tests/system/fixtures/simple/compile_commands.json abstractness instability distance_from_main_sequence --module=namespace
  ```
  produces a Markdown report with correct values matching system test expectations.
- All CI gates pass.
- Docs exist: README, user manual, developer guide, ADRs for non-trivial decisions.
- Adding a new metric requires:
  - New metric class + registration
  - Unit tests
  - System test update
  - No changes to core analysis pipeline interfaces (ideal case)

---

## 13. Roadmap Principles (for TASKS.md)

- Each increment must be:
  - Small scope
  - Runnable CLI
  - Includes an automated system test
  - Includes/updates ADR if a design decision is made
