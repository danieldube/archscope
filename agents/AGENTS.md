# Agent Hints

- Read `agents/TASKS.md` first and work the next open task in order. Do not skip ahead.
- Use `agents/SPEC.md` as the source of truth for architecture, CLI behavior, and boundaries between `archscope`, `archscope_core`, and future Clang-specific pieces.
- Follow test-first development: add or update unit/system tests before implementation.
- Keep the project compatible with C++17 and preserve the current CMake target layout.
- Keep core logic free of CLI and filesystem concerns where practical; prefer small, testable units.
- Prefer local scripts for verification: `./scripts/configure.sh`, `./scripts/build.sh`, `./scripts/test.sh`, `pre-commit run --all-files`.
- Formatting and `clang-tidy` are enforced through `pre-commit`; C/C++ style comes from `.clang-format`.
- Avoid network-dependent solutions. The repository is intended to build and test locally.
- Update docs when behavior or developer workflow changes, especially `README.md` and `docs/developer-guide.md`.
- Record concrete process mistakes and fixes in `agents/LEARNINGS.md` so later runs avoid repeating them.
