# US-1302: Page-fault comparison for replacement algorithms

## Requirement

As a performance analyst, I want page-fault results for each algorithm so that replacement effectiveness can be compared.

## Scope

This requirement ensures that equivalent benchmarking workloads can be executed under each supported replacement policy and that each policy's page-fault performance is recorded with comparable totals, rates, and ordering.

## Implementation choices

- Page-fault statistics remain captured by the MMU and its existing `PageFaultStatistics` collector.
- A dedicated comparison helper ranks the recorded results by page-fault count so direct comparisons remain stable and repeatable.
- A simulation-level benchmark comparison runner executes the same workload against FIFO, LRU, Clock, and Optimal policies.
- The comparison output is designed to integrate with the existing benchmark configuration and statistics infrastructure without introducing policy-specific logic outside the simulation wrapper.

## Interpretation limitation

The comparison runner executes FIFO, LRU, Clock, and Optimal for the same benchmark parameters and seeded workload. However, the normal `Simulation` path does not provide the future-reference sequence required by `OptimalPageReplacementPolicy`. The reported “Optimal” run is therefore not a theoretical optimal baseline. Do not make claims about Optimal's minimum possible fault count from this runner until future-trace injection is implemented.

## Expected outputs

- Per-policy page-fault totals for the same workload
- Per-policy fault-rate values in the range `[0, 1]`
- Stable ordering by fault count for direct comparison
- Human-readable summary output for analyst reporting

## Verification evidence

- Unit verification is in `tests/unit/statistics/PageFaultComparisonTest.cpp`.
- The system-level controlled benchmark tests exercise equivalent configuration and access totals; they do not establish a published numeric performance result.
- Broader test coverage and commands are documented in [Testing and performance](../verification/testing-and-performance.md).
