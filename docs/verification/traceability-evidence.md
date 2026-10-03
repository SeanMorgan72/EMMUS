# Traceability evidence workflow

This page describes how to keep the [requirements matrix](../requirements/requirements-traceability-matrix.md) useful during implementation and review.

## Evidence model

For each requirement, record:

1. **Implementation:** public interface and/or implementation files that fulfill the behavior.
2. **Test:** relevant unit, integration, system, and optional view test files.
3. **Verification:** the reproducible command and outcome for the test suite actually run.
4. **Gap:** any missing layer, assumption, unsupported behavior, or limitation that affects interpretation.

Evidence should point to current paths in the source tree. A class existing is not proof of complete behavior; test filenames are not proof that tests passed.

## Review workflow

1. Add or revise the story/requirement summary in the matrix.
2. Link the implementation and the most relevant test sources.
3. Run the focused test label after building, then the full CTest suite when appropriate.
4. Record observed result and unresolved coverage/quality limitations without claiming unperformed verification.
5. Update architecture or workflow pages when user-visible behavior or developer instructions change.

Core test labels and commands are described in [Testing and performance](testing-and-performance.md); configure/build/install instructions are in [Build and development workflow](../development/build-and-workflow.md).

## Current evidence state

- Unit, integration, and system test source trees exist and are required by the normal test-enabled CMake configuration.
- System tests exercise complete headless simulations; `tests/system` is not empty.
- The optional GUI-view test is built only when `EMMUS_BUILD_GUI=ON`.
- The CI workflow runs Debug and Release configure/build/test/install steps.
- The standard CI workflow does not run performance benchmarks.
- There is no automated requirement-matrix or Markdown-link validation.
- The project contains no checked-in numeric benchmark-result dataset.

The [matrix](../requirements/requirements-traceability-matrix.md) records known requirement-level gaps, including the absence of future-trace injection for Optimal during normal simulations.
