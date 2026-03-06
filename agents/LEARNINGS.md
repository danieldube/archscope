# Session Learnings

1. Verify the repository state before selecting the next roadmap item.
   `agents/TASKS.md` is ordered, but completion must be determined from the
   actual code and tests, not from assumptions about prior progress.
2. Probe tool and dependency availability early. This session lacked `rg`,
   Catch2, and CLI11, so checking local capabilities up front avoided a
   dead-end implementation plan.
3. Keep the initial `clang-tidy` baseline practical for the current increment.
   Enabling style-heavy checks too early creates noisy, low-value churn that
   distracts from shipping the requested behavior.
4. Probe preferred shell tools explicitly before using them in the first
   command. This session assumed `rg` was present; use `command -v rg >/dev/null`
   and fall back immediately to `find` or `grep` when it is not.
5. Do not run `clang-format` on `CMakeLists.txt` or `.cmake` files. Use a
   CMake-aware formatter, or leave them untouched unless edited manually,
   because the C/C++ formatter can corrupt CMake syntax.
6. In CMake `execute_process()`, pass `--flag=${value}` as a single argument
   without embedding shell quotes inside the string. Literal quote characters
   become part of the argv value and can silently redirect outputs to the wrong
   path.
