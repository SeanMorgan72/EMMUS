# US-1303: Execution-time measurement and overhead comparison

## Requirement

As a performance analyst, I want execution-time measurements so that computational overhead can be evaluated.

## Scope

This requirement adds a reusable execution-time measurement and comparison framework for equivalent simulation workloads across page-replacement algorithms. The implementation must collect high-resolution timing data, capture per-policy overhead, and surface consistent reporting for direct comparison.

## Implementation choices

- Each policy already records replacement-level timing in the existing `PageReplacementStatistics` collector.
- A dedicated execution-time comparison object ranks results by average replacement time while preserving each policy's total simulation runtime.
- The simulation benchmark helper runs the same benchmark under FIFO, LRU, Clock, and Optimal policies and records each result with the matching elapsed wall-clock time.
- The executor no longer emits per-access debug output on the hot path so instrumentation overhead does not distort the measured timing.

## Expected outputs

- Per-policy total replacement time and average replacement time
- Per-policy total simulation time for the same workload
- Stable ranking by replacement overhead for direct comparison
- Human-readable summary output for analyst reporting
- Repeatable measurements across equivalent workloads and policy runs

## Verification evidence

- Unit verification is in `tests/unit/statistics/ExecutionTimeComparisonTest.cpp`.
- Benchmark integration is exercised via `SimulationComparison::runExecutionTimeComparison()`.
- The existing page-replacement statistics tests remain the baseline for timing aggregation semantics.
