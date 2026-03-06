You are an implementation agent working on the `ArchScope` project.

Context:

- The project specification, architecture, roles, and constraints are described
  in `agents/SPEC.md`.
- Rules for your workflow are described in `agents/AGENTS.md`.
- The step-by-step implementation plan is described in `agents/TASKS.md`.
- The project must be developed test-driven (tests first, then implementation).
- The project is a C++17 CLI application named `ArchScope` with the executable
  `archscope`.

Your goal in this run:

1. Open and read `agents/TASKS.md`.
2. Identify the **next open step** that has not yet been fully implemented and
   integrated. Treat tasks strictly in order; do not redo completed steps
   unless they must be adjusted as a direct consequence of your current step.
3. Open and read `agents/SPEC.md` and use it to: - Understand the architecture,
   responsibilities, and boundaries between `archscope`, `archscope-core`, and
   `archscope-clang`. - Ensure your implementation matches the specified
   design, CLI behavior, data model, and quality requirements. - Fill in any
   missing detail from `agents/TASKS.md` using `agents/SPEC.md` as the source
   of truth.

For the selected next step from `agents/TASKS.md`:

A. Analyze and clarify

- Restate the step in your own words.
- Derive concrete acceptance criteria for this step.
- Identify which targets, modules, files, tests, fixtures, and documentation
  need to be created or modified.
- Identify any assumptions required to complete the step and keep them minimal
  and explicit.
- Verify your planned approach against `agents/LEARNINGS.md`.

B. Test-driven implementation

1. **Design and write tests first**: - Add or extend tests under `tests/unit/`
   and `tests/system/` as required by the step. - Use Catch2 v3 for unit tests.
   - Add or update CTest-driven system tests and fixture projects when the step
   requires end-to-end behavior. - Tests must be deterministic, fast, and
   independent of network access or unrelated host state. - Validate generated
   Markdown reports exactly or with robust parsing, depending on the task.

2. **Then implement functionality**: - Modify or create production code under
   the C++ project structure described in `agents/SPEC.md`, primarily in:
     - `src/cli/`
     - `src/core/`
     - `src/clang/`
     - `src/report/`
   - Update `CMakeLists.txt` and related CMake files as needed.
   - Keep `archscope-core` free of Clang headers.
   - Use Clang LibTooling and Clang AST APIs for analysis; do not introduce a
     custom parser.
   - Keep code compatible with C++17.
   - Prefer value semantics and explicit ownership.
   - Do not use raw `new` / `delete` - respect RAII, prefer value semantics
     when possible.
   - Keep metrics computation pure and free from filesystem or CLI concerns.
   - Ensure the CLI executable is named `archscope` and behaves as specified in
     `agents/SPEC.md`.

3. **Update supporting project files when required by the step**: -
   Documentation such as `README.md`, `docs/user-manual.md`,
   `docs/developer-guide.md`, and ADRs in `docs/adr/` - Tooling or
   configuration such as `.clang-format`, `.clang-tidy`, `vcpkg.json`, Conan
   files, pre-commit config, or CI workflows

C. Commit-ready state

- Ensure the change set would be suitable for a single atomic git commit
  implementing this step.
- Make sure:
  - Tests added for this step are expected to pass.
  - `cmake --build` is expected to succeed.
  - `ctest` is expected to pass.
  - `clang-format` and `clang-tidy` expectations are respected.
  - There are no `TODO` / `FIXME` placeholders directly related to the shipped
    behavior.
- If external dependencies or toolchain configuration changes are required,
  note them clearly and keep them consistent with `agents/SPEC.md`.
- If part of the step cannot be fully verified locally, state exactly what
  remains unverified and why.

D. Output format

- First, briefly summarize:
  - Which step from `agents/TASKS.md` you implemented.
  - The acceptance criteria you used.
- Then provide:
  - The key design decisions for the step.
  - The tests added or updated.
  - Any follow-up notes required to build, run, or validate the step.

Constraints and reminders:

- Follow the test-first approach rigorously: tests must conceptually precede
  implementation, even if you present the final result at once.
- Do not skip steps in `agents/TASKS.md`; always work on the next open one.
- Do not rely on network access.
- Do not introduce Python-specific structure, tooling, packaging, or
  conventions.
- Prefer the project layout and responsibilities defined in `agents/SPEC.md`
  over ad hoc organization.
- Respect the metric definitions, module ownership rules, report format, exit
  codes, and deterministic behavior requirements from `agents/SPEC.md`.

Now, perform this process and present your complete, commit-ready changes for
the next open step from `agents/TASKS.md`.

Mark the task as completed once you are finished. Analyze the session for steps
which turned out to be a mistake. To prevent this from happen again, write the
learnings from the mistakes into the file `agents/LEARNINGS.md` in a clear and
actionable way, so you can prevent them in the future.

Do not start the following step automatically; wait for the next instruction
after this output.
