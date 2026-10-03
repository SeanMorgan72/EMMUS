# US-1303: Execution-time measurement and overhead comparison

## Requirement

As a performance analyst, I want execution-time measurements so that computational overhead can be evaluated.

## Scope

This requirement adds a reusable execution-time measurement and comparison framework for equivalent simulation workloads across page-replacement algorithms. The implementation records `steady_clock` durations in nanoseconds, captures per-policy replacement-decision time and total simulation time, and surfaces a summary for comparison. The nanosecond representation does not guarantee nanosecond accuracy or resolution.

## Implementation choices

- Each policy already records replacement-level timing in the existing `PageReplacementStatistics` collector.
- A dedicated execution-time comparison object ranks results by average replacement time while preserving each policy's total simulation runtime.
- The simulation benchmark helper runs the same benchmark under FIFO, LRU, Clock, and Optimal policies and records each result with the matching elapsed wall-clock time.

## Expected outputs

- Per-policy total replacement time and average replacement time
- Per-policy total simulation time for the same workload
- Stable ranking by replacement overhead for direct comparison
- Human-readable summary output for analyst reporting
- Equivalent workload parameters and seeded generation across policy runs; repeated statistical measurement is left to the caller.

The comparison helper sorts results from one execution; it does not itself repeat runs or produce a performance distribution. No checked-in measured-results dataset, benchmark executable, or CI performance threshold currently exists. Normal simulations also do not provide the future trace required by Optimal, so its timing can be compared as an implementation run but its fault effectiveness must not be called theoretically optimal.

## Verification evidence

- Unit verification is in `tests/unit/statistics/ExecutionTimeComparisonTest.cpp`.
- Benchmark integration is exercised via `SimulationComparison::runExecutionTimeComparison()`.
- The existing page-replacement statistics tests remain the baseline for timing aggregation semantics.
- Measurement boundaries, limitations, and reproducibility guidance are in [Testing and performance](../verification/testing-and-performance.md).
