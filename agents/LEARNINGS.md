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
7. Do not run build and test commands in parallel against the same build
   directory when `ctest` depends on freshly built targets. Finish the build
   first, then run tests, or `ctest` can pick up stale executables and report
   misleading failures.
8. When feeding arbitrary paths through `xargs sh -c`, avoid `printf` formats
   that treat the path as an option. Use `printf --` or a simpler direct `sed`
   loop so paths beginning with `-` do not break the inspection command.
9. In sandboxed runs, do not assume `pre-commit` can use the default cache
   directory or fetch missing hook repositories. Set `PRE_COMMIT_HOME` to a
   writable location first, and if the environment is offline, fall back to
   direct local tools like `clang-format`, `clang-tidy`, and `ctest`.
10. Do not run overlapping verification commands that execute the same tests in
    parallel when those tests use filesystem fixtures or temp directories.
    Either verify serially, or make the tests use unique per-run paths so
    concurrent runs cannot race and produce false failures.
11. Keep CI system packages explicit when CMake depends on distro-provided
    LLVM/Clang configs. `find_package(Clang CONFIG)` can locate
    `/usr/lib/llvm-18/lib/cmake/clang/ClangTargets.cmake` from a partial image,
    but the configure still fails unless `libclang-18-dev`, `llvm-18-dev`, and
    related native dependencies such as `libcurl4-openssl-dev` are installed.
