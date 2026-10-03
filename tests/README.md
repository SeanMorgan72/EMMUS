# EMMUS Automated Testing Framework

The EMMUS automated test framework uses:

- C++23
- GoogleTest
- CMake
- CTest

The framework follows the EMMUS architecture's three verification levels:

1. Unit tests
2. Integration tests
3. System tests


# Directory Structure

    tests/
    ├── CMakeLists.txt
    │
    ├── data/
    │   └── README.md
    │
    ├── fixtures/
    │   ├── TestFixture.hpp
    │   ├── MemoryManagerFixture.hpp
    │   ├── PageReplacementAlgorithmFixture.hpp
    │   └── SimulationFixture.hpp
    │
    ├── unit/
    │   ├── algorithms/
    │   │   └── replacement/
    │   ├── application/
    │   ├── infrastructure/
    │   ├── memory/
    │   │   ├── mmu/
    │   │   ├── virtual/
    │   │   └── physical/
    │   ├── simulation/
    │   ├── statistics/
    │   └── framework/
    │
    ├── integration/
    │   └── framework/
    │
    └── system/
        └── framework/


# Building

Configure the project:

    cmake --preset debug

Build every enabled test target:

    cmake --build --preset debug


# Running Tests

Run the complete discovered suite and fail if no tests were registered:

    ctest --preset debug --no-tests=error


# Running Unit Tests

    ctest --test-dir build/debug -L unit --output-on-failure


# Running Integration Tests

    ctest --test-dir build/debug -L integration --output-on-failure


# Running System Tests

System tests are included in the default test build. Run them with:

    ctest --test-dir build/debug -L system --output-on-failure


# GoogleTest Discovery

Tests are automatically discovered using:

    gtest_discover_tests()

This means individual GoogleTest cases do not need to be manually added to
CTest.

For example:

    TEST(MyTest, DoesSomething)

automatically becomes a CTest test after the test executable is built.

Unit, integration, and system test sources are required at configure time.
This prevents a missing test tier from being mistaken for a complete suite.
CI also uses CTest's `--no-tests=error` option so an empty discovery result
cannot pass silently.


# Testing Conventions

## 1. One Behavior Per Test

Each test should verify one meaningful behavior.

Prefer:

    TEST(FIFOPageReplacementPolicyTest, SelectsOldestFrame)

over a test that validates many unrelated behaviors.


## 2. Arrange / Act / Assert

Tests should generally follow:

    // Arrange

    // Act

    // Assert


## 3. EXPECT vs ASSERT

Use EXPECT_* when the test can continue after a failure.

Use ASSERT_* when the remaining test requires the condition to be true.


## 4. Test Public Behavior

Tests should verify externally observable behavior rather than private
implementation details.


## 5. Independent Tests

Each test should be independent.

Avoid:

- Global mutable state.
- Test execution ordering.
- Shared objects between tests.
- Hidden dependencies between test cases.


## 6. Fresh State

Construct fresh production objects for each test whenever practical.

Use SetUp() only for genuinely common initialization.


## 7. Determinism

Correctness tests should be deterministic.

Randomized workloads must use explicit random seeds.


## 8. Performance Tests

Performance tests should remain separate from correctness tests.

Correctness tests should establish expected behavior.

Performance tests should measure:

- Large workloads.
- Replacement decision time.
- Simulation throughput.
- Memory consumption where practical.


## 9. Test Naming

Test names should describe observable behavior.

Examples:

    ChooseVictimReturnsOldestResidentFrame

    AccessUpdatesRecentUseInformation

    InvalidVirtualAddressIsRejected

    PageFaultLoadsPageIntoAvailableFrame

    DirtyPageIsRecordedWhenEvicted


## 10. Requirement Traceability

Where practical, tests should identify the requirement they verify.

Example:

    // FR-010: FIFO Page Replacement

    TEST(
        FIFOPageReplacementPolicyTest,
        FR010_SelectsOldestResidentFrame
    )


# Unit-Test Mapping

| Test Area | Primary Verification |
|-----------|----------------------|
| Page | Page state and behavior |
| PageTable | Mapping and residency |
| Frame | Frame state and ownership |
| PhysicalMemory | Allocation and capacity |
| MMU | Address translation |
| MMU | Page hits and page faults |
| FIFO | FIFO replacement behavior |
| LRU | LRU replacement behavior |
| Clock | Reference-bit behavior |
| Optimal | Future-use victim decisions |
| Factory | Runtime policy selection |
| Configuration | Configuration validation |
| Statistics | Fault/replacement statistics |
| Statistics | Dirty eviction statistics |
| Statistics | Timing statistics |
| Workload | Deterministic workload generation |


# Integration-Test Mapping

Integration tests should verify interactions between:

- MMU + PageTable
- MMU + PhysicalMemory
- MMU + PageReplacement
- Process + Workload + MMU
- Simulation + Statistics


# System-Test Mapping

System-level tests verify complete simulation scenarios:

- Complete workload execution.
- Replacement-algorithm comparison.
- Page-fault comparison results for each algorithm.
- Expected page-fault counts.
- Dirty-page behavior.
- Configuration-driven execution.


# Testing Principle

Production code must never depend on test code.

Test code may depend on production interfaces and components.

The core simulation engine should remain testable without the GUI.