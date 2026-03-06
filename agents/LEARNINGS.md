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
