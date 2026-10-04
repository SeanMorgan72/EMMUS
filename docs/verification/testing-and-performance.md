# Testing, verification, and performance

## Verification strategy

EMMUS uses GoogleTest for assertions, CMake for test executable construction/discovery, and CTest for execution and labels. The verification layers target different risks:

| Layer | Scope | Representative evidence |
|---|---|---|
| Unit | One component or value type and its observable contract. | Policy decision behavior, Page state transitions, MMU access cases, configuration validation, statistics aggregation, workload generation. |
| Integration | Collaborating core components. | MMU + PageTable + PhysicalMemory + policy, configuration-driven simulation, process/workload interactions, simulation result/statistics. |
| System | Complete configured headless simulation runs. | Sequential and seeded-random runs, locality and mixed workloads, multi-process execution, invalid configuration, repeatability, policy selection, equal benchmark access totals. |
| Optional view | Text presentation of snapshots and events (enabled only with `EMMUS_BUILD_GUI=ON`). | Empty/reset state, progress and completion rendering, memory utilization, policy names, comparisons, all four policy labels. |

The core tests do not require an application GUI. The tests are discovered by CMake's `gtest_discover_tests()` and tagged `unit`, `integration`, `system`, or (when enabled) `gui`. Test sources are required at configure time for each core tier; CTest's `--no-tests=error` prevents no discovered tests from being mistaken for success.

The default build also registers `emmus-demo-smoke` when `EMMUS_BUILD_DEMO=ON`. It runs the portfolio walkthrough, including controlled FIFO/LRU/Optimal page-fault assertions; it complements rather than replaces the layered GoogleTest suites.

## What the tests establish

- **Memory primitives:** Page state, frame allocation/release, typed access records, page-table mappings, and mapping consistency.
- **MMU behavior:** Resident accesses, faults, use of free frames, victim selection when full, dirty victim accounting, invalid/unregistered/foreign process accesses, and consistency checks.
- **Policies:** Direct FIFO, LRU, Clock, and Optimal decision behavior. Optimal unit and integration cases explicitly provide a future-reference sequence.
- **Workloads and processes:** Seeded randomness, locality and mixed generation, per-process access composition, and lifecycle operations.
- **Statistics/comparisons:** Fault count/rate aggregation, replacement count/dirty eviction/timing aggregation, rankings, readable summaries, and comparison execution across policies.
- **End-to-end:** Complete simulations without the GUI, expected record counts, same-seed behavior, workload/configuration validation, and policy configuration.
- **Presentation:** `SimulationStatisticsVisualizationView` tests exercise progress snapshots, completed result mapping, rendered counters, baseline comparison, and clearing state.

Test names and detailed cases are the source for precise behavioral coverage; see [tests/README.md](../../tests/README.md) and [system-test scenarios](../../tests/system/README.md). A test file's presence is evidence that behavior is exercised, not a claim of exhaustive proof.

## Reproducing verification

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug --no-tests=error
```

Run an individual tier after building:

```sh
ctest --test-dir build/debug -L unit --output-on-failure
ctest --test-dir build/debug -L integration --output-on-failure
ctest --test-dir build/debug -L system --output-on-failure
```

For the optional text-view test, configure a separate tree with `-DEMMUS_BUILD_GUI=ON`, build it, then run `ctest --test-dir build/gui -L gui --output-on-failure`. The exact commands, configuration options, toolchain expectations, Release run, and CI sequence are in [Build and development workflow](../development/build-and-workflow.md).

## Performance instrumentation currently implemented

EMMUS has measurement and comparison **code**, distinct from a published performance study:

| Measurement | How it is produced | Interpretation |
|---|---|---|
| Total simulation elapsed time | `Simulation::run()` measures with `std::chrono::steady_clock` and stores a nanosecond duration in the result. | Whole run duration includes setup and workload creation/execution. It is machine-, compiler-, build-, and workload-dependent. |
| Replacement decision duration | Each policy times `chooseVictim()` with `steady_clock` and records elapsed nanoseconds in `PageReplacementStatistics`. | The measured interval is the victim-choice method only, not the full fault/eviction path. Short intervals can approach clock granularity and instrumentation overhead. |
| Average replacement time | Accumulated replacement duration divided by number of recorded replacements; zero if no replacement was recorded. | A descriptive average, not a statistically robust latency distribution. |
| Page-fault comparison | `SimulationComparison::run()` reruns one `BenchmarkConfiguration` across FIFO, LRU, Clock, and Optimal, recording totals and fault rates. | Seed/configuration preserve workload comparability. Normal-run Optimal lacks future references (see below). |
| Timing comparison | `runExecutionTimeComparison()` repeats the same policy set and ranks by average replacement-decision time, retaining total simulation time. | Ranking is a single-run result unless a caller repeats experiments; ties have deterministic policy-order tie-breaking. |

`BenchmarkConfiguration` defines page size, frame count, process/page counts, total accesses, workload type, seed, and policy. Each compared policy gets a configuration copy with only its policy changed. See [page-fault comparison](../requirements/page-fault-comparison.md) and [execution-time comparison](../requirements/execution-time-comparison.md) for the requirement-level design notes.

## Results available and not available

The repository contains automated tests that assert measurement fields are non-negative, validate comparison result coverage, check deterministic workload sizes, and verify ranking/summary behavior using controlled synthetic statistics. The tests do **not** publish a stable table of elapsed times, throughput, memory consumption, repeated-run distributions, or speedup claims.

There is no dedicated benchmark executable, checked-in performance-results file, or documented controlled measurement run. Therefore this documentation reports **no numeric performance result**. Build artifacts and historical CTest logs are not treated as a reproducible benchmark dataset. Before claiming relative performance, run repeated Release experiments on a recorded machine/toolchain with an identical workload and seed; report repetitions and variation, and separate fault effectiveness from runtime cost.

## Interpretation caveats

1. **Optimal needs a future trace.** Policy tests directly set a reference sequence, but `Simulation` does not currently derive and pass one. An Optimal result from `SimulationComparison` is not theoretically optimal and should be labelled accordingly.
2. **Timing is not a benchmark protocol.** Nanosecond storage does not guarantee nanosecond resolution or accuracy. A one-shot runtime includes setup; policy-level timing is narrower and can be dominated by clock-call overhead for fast decisions.
3. **No memory-footprint metric.** Statistics do not measure process RSS, allocations, cache behavior, or simulated byte volume.
4. **Tests are correctness checks.** CI builds and tests Debug and Release, but CI does not run a performance regression threshold.
5. **Scope is simulated.** Results describe this implementation and workload model, not a real operating system or physical memory device.

## Traceability status

Core unit, integration, and system test suites are present. Some individual requirements have only unit-level coverage or no dedicated end-to-end case; those gaps are recorded in the [requirements matrix](../requirements/requirements-traceability-matrix.md). No automated documentation-link or matrix-consistency checker is configured. See the [evidence workflow](traceability-evidence.md) for maintenance expectations.
