# ADR 0002: Fetch Catch2 with CMake for test builds

- Status: Accepted

## Context

The bootstrap scaffold used a local Catch2-compatible shim to keep the initial
increment buildable without depending on package installation or downloads.
That shim is now a liability:

- It diverges from real Catch2 behavior and API coverage.
- It forces ArchScope to maintain bespoke test infrastructure.
- The project already uses CMake as the build orchestration layer, so test
  dependencies can be managed there directly.

We now want the unit test executable to use the real Catch2 v3 framework and
have CMake obtain it during configure time.

## Decision

Use CMake `FetchContent` to retrieve Catch2 v3 for test builds.

- `BUILD_TESTING` gates the Catch2 fetch and test-only wiring.
- `archscope_tests` links against `Catch2::Catch2WithMain`.
- CTest test registration uses Catch2's `catch_discover_tests`.
- The temporary local Catch2 shim and custom test main are removed.

## Alternatives considered

- Keep the local shim: rejected because it provides incomplete behavior and
  increases maintenance cost.
- Use vcpkg for Catch2: rejected because this decision standardizes Catch2
  acquisition in CMake rather than through the external package manifest.
- Depend on a system-installed Catch2: rejected because it makes developer and
  CI environments less reproducible.

## Consequences

- Test behavior now matches real Catch2 v3 semantics.
- CMake becomes the source of truth for obtaining the test framework.
- Configuring tests now requires network access, or a pre-populated CMake
  download cache / mirrored dependency source.
- The bootstrap ADR is superseded, but the overall target layout remains
  unchanged.
