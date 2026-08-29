# EMMUS Test Data

This directory contains reusable deterministic workloads and configuration
data used by the automated test suite.

## Recommended Test Workloads

### FIFO Anomaly

Workloads demonstrating FIFO page-replacement behavior and possible FIFO
anomalies.

Recommended location:

    tests/data/fifo_anomaly/

### LRU Locality

Workloads demonstrating locality and recently-used page behavior.

Recommended location:

    tests/data/lru_locality/

### Clock Reference Bit

Workloads designed to exercise reference-bit behavior and clock-hand
movement.

Recommended location:

    tests/data/clock_reference/

### Optimal Benchmark

Known access sequences for which expected optimal replacement decisions can
be calculated.

Recommended location:

    tests/data/optimal_benchmark/

### Configuration

Valid and invalid configuration examples.

Recommended location:

    tests/data/configuration/

## Test Data Rules

Test data should be:

1. Deterministic.
2. Small enough to understand.
3. Version controlled.
4. Reusable across appropriate tests.
5. Documented when the expected behavior is not obvious.

Randomized workloads should use an explicit random seed so that failures can
be reproduced.