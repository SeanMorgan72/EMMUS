# System Tests

System tests exercise complete EMMUS simulation workflows without requiring
the optional GUI.

## Implemented scenarios

- `CompleteSimulationExecutionTest.cpp` verifies configured sequential and
  seeded random workloads complete and emit the requested number of accesses.
- `ControlledBenchmarkSystemTest.cpp` compares consistent benchmark output
  across replacement policies and checks benchmark configuration behavior.
- `EndToEndSimulationSystemTest.cpp` verifies sequential, multi-process random,
  locality, and mixed workloads; invalid configuration handling; same-seed
  repeatability; and all registered replacement policies.
- `SystemFrameworkSmokeTest.cpp` verifies the system-test framework is linked.

All system tests are automatically discovered from the test executable and
registered with CTest under the `system` label.

## Traceability expectation

System tests should identify the requirement or end-to-end behavior they
verify. Where a requirement ID is available, include it in the test name or
test documentation.
