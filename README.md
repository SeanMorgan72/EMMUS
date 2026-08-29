# Enhanced Memory Management Unit Simulator (EMMUS)

**Enhanced Memory Management Unit Simulator (EMMUS)** is a C++23 software and systems engineering project that simulates fundamental computer memory-management operations.

EMMUS models the interaction between virtual memory, physical memory, page tables, processes, memory accesses, page faults, page replacement, and memory-management statistics.

The project is being developed as a **software and systems engineering portfolio project** to demonstrate the complete engineering lifecycle of a non-trivial software system—from requirements and architecture through implementation, verification, validation, and performance evaluation.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Project Objectives](#project-objectives)
- [Key Features](#key-features)
- [Page-Replacement Algorithms](#page-replacement-algorithms)
- [System Architecture](#system-architecture)
- [Technology Stack](#technology-stack)
- [Repository Structure](#repository-structure)
- [Requirements](#requirements)
- [Building EMMUS](#building-emmus)
- [Running EMMUS](#running-emmus)
- [Running Tests](#running-tests)
- [Testing Strategy](#testing-strategy)
- [Memory-Management Model](#memory-management-model)
- [Workloads](#workloads)
- [Performance Evaluation](#performance-evaluation)
- [Engineering Process](#engineering-process)
- [Project Documentation](#project-documentation)
- [Development Status](#development-status)
- [Future Enhancements](#future-enhancements)
- [Design Principles](#design-principles)
- [Portfolio Objectives](#portfolio-objectives)
- [Limitations](#limitations)
- [Getting Started](#getting-started)
- [Contributing](#contributing)
- [License](#license)
- [Author](#author)

---

## Project Overview

An operating system must manage virtual memory while efficiently using the available physical memory.

When a process accesses a virtual address, the memory-management system must determine the corresponding physical location. If the required page is not currently resident in physical memory, a **page fault** occurs.

When physical memory is full, the system must determine which resident page should be removed to make room for the requested page.

EMMUS provides a controlled software environment for modeling these operations and evaluating different page-replacement strategies.

At a conceptual level, a memory access follows this process:

```text
Process
   |
   v
Memory Access
   |
   v
Virtual Address
   |
   v
Page Table Lookup
   |
   +----------------------+
   |                      |
   v                      v
Page Resident?         Page Fault
   |                      |
  Yes                     v
   |                 Free Frame?
   |                  /         \
   |                Yes          No
   |                 |            |
   |                 v            v
   |             Load Page   Select Victim
   |                              |
   |                              v
   |                         Evict Page
   |                              |
   |                              v
   |                          Load Page
   |                              |
   +--------------+---------------+
                  |
                  v
         Physical Address
                  |
                  v
          Continue Simulation
```

The simulator is designed to separate memory-management responsibilities into modular components that can be developed, tested, and evaluated independently.

---

## Project Objectives

The primary objectives of EMMUS are to:

- Model fundamental virtual-memory concepts.
- Simulate virtual-to-physical address translation.
- Model physical memory and frame allocation.
- Manage page tables.
- Detect and process page faults.
- Implement multiple page-replacement algorithms.
- Model dirty-page behavior.
- Support multiple simulated processes.
- Generate configurable memory-access workloads.
- Collect simulation statistics.
- Compare page-replacement algorithms.
- Provide automated software verification.
- Support reproducible experiments.
- Evaluate algorithm performance.
- Demonstrate professional software engineering practices.

---

## Key Features

The EMMUS project is designed to provide the following capabilities:

- Virtual-memory simulation
- Physical-memory simulation
- Page-table management
- Physical-frame management
- Virtual-to-physical address translation
- Page-fault detection
- Page-fault handling
- Page replacement
- Dirty-page handling
- Multi-process memory management
- Configurable memory-access workloads
- Runtime replacement-policy selection
- Simulation statistics
- Automated unit testing
- Integration testing
- System testing
- Regression testing
- Performance evaluation

---

## Page-Replacement Algorithms

EMMUS uses a common replacement-policy abstraction so that multiple algorithms can be implemented and evaluated using the same memory-management infrastructure.

The initial algorithms are:

| Algorithm | Description |
|---|---|
| **FIFO** | Replaces the page that has been resident in physical memory the longest. |
| **LRU** | Replaces the page that has been least recently accessed. |
| **Clock** | Uses reference information and a circular replacement structure to approximate LRU behavior. |
| **Optimal** | Uses knowledge of future memory references to select the theoretically optimal victim. |

### FIFO — First-In, First-Out

FIFO maintains pages according to their loading order.

When replacement is required, the page that has been resident the longest is selected.

FIFO is simple and has relatively low tracking overhead, but it does not consider how frequently or recently pages are accessed.

### LRU — Least Recently Used

LRU maintains information about page-access history.

When replacement is required, the page that has not been accessed for the longest period of time is selected.

LRU is designed to take advantage of temporal locality.

### Clock

Clock uses a circular structure and reference information to provide pages with a second chance before they are selected for replacement.

The algorithm is intended to approximate LRU behavior while reducing some of the tracking overhead associated with maintaining exact access ordering.

### Optimal

Optimal uses knowledge of future memory references to determine which page should be replaced.

The page whose next reference occurs furthest in the future is selected, or a page that will not be referenced again may be selected according to the implementation rules.

Because a real operating system generally cannot know future memory references, Optimal is primarily useful as a theoretical benchmark.

---

## System Architecture

EMMUS is organized around separation of responsibilities.

A simplified architectural view is:

```text
+------------------------------------------------+
|                Application Layer               |
+------------------------------------------------+
|                Simulation Layer                |
+------------------------------------------------+
|          Memory Management / MMU Layer          |
+------------------------------------------------+
|     Page Tables / Physical Memory / Frames     |
+------------------------------------------------+
|          Replacement Policy Interface          |
+------------------------------------------------+
| FIFO | LRU | Clock | Optimal Implementations  |
+------------------------------------------------+
|             Domain / Infrastructure            |
+------------------------------------------------+
```

The architecture is intended to provide:

- Separation of concerns
- Encapsulation
- Testability
- Extensibility
- Maintainability
- Clear subsystem boundaries

The replacement-policy abstraction allows the MMU and pager to work with different algorithms without becoming tightly coupled to a specific implementation.

---

## Technology Stack

| Technology | Purpose |
|---|---|
| **C++23** | Primary programming language |
| **CMake** | Build-system configuration |
| **Ninja** | Build execution |
| **GoogleTest** | Automated unit and integration testing |
| **Git** | Version control |
| **YAML** | Configuration data, where applicable |
| **Qt** | Graphical user interface, where applicable |

The final project configuration will document the specific dependency versions used for the completed implementation.

---

## Repository Structure

The repository is organized approximately as follows:

```text
EMMUS/
├── .github/
├── apps/
├── cmake/
├── data/
├── docs/
├── include/
├── src/
├── tests/
├── CMakeLists.txt
└── README.md
```

### `.github/`

Contains GitHub-specific configuration, including continuous-integration workflows where applicable.

### `apps/`

Contains application-level source code and application entry points.

### `cmake/`

Contains CMake modules and supporting build configuration.

### `data/`

Contains workloads, configuration files, and other simulation data.

### `docs/`

Contains project engineering documentation.

### `include/`

Contains public C++ header files.

### `src/`

Contains C++ implementation files.

### `tests/`

Contains automated tests, test fixtures, and supporting test data.

### `CMakeLists.txt`

Defines the project's CMake build configuration.

### `README.md`

Provides the public-facing overview and entry point for the project.

---

## Requirements

A development environment should provide:

- A C++23-compatible compiler
- CMake
- Ninja
- Git
- GoogleTest
- Required project dependencies

The exact versions used for the completed project should be documented in the repository.

---

## Building EMMUS

### Configure a Debug Build

A typical Debug configuration is:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### Build the Project

```bash
cmake --build build
```

### Configure a Release Build

A Release build can be configured using:

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Build the Release Configuration

```bash
cmake --build build-release
```

The Release configuration should generally be used for final performance measurements.

---

## Running EMMUS

The final executable name and command-line interface will be documented once the application interface has been finalized.

The general execution model is:

```bash
./<emmus-executable>
```

Depending on the final implementation, the application may support options for:

- Configuration files
- Workloads
- Replacement algorithms
- Number of physical frames
- Page size
- Number of processes
- Workload parameters
- Logging level
- Output configuration

Example usage will be added when the command-line interface is finalized.

---

## Running Tests

EMMUS uses automated testing to verify individual components, subsystem interactions, and complete system behavior.

A typical command for running the complete CTest suite is:

```bash
ctest --test-dir build --output-on-failure
```

Individual test executables may also be run directly when debugging or developing specific test suites.

---

## Testing Strategy

Testing follows a layered verification strategy.

```text
                 +----------------+
                 |   Unit Tests   |
                 +--------+-------+
                          |
                          v
                 +----------------+
                 | Integration    |
                 |     Tests      |
                 +--------+-------+
                          |
                          v
                 +----------------+
                 |  System Tests  |
                 +--------+-------+
                          |
                          v
                 +----------------+
                 |   Regression   |
                 |     Tests      |
                 +--------+-------+
                          |
                          v
                 +----------------+
                 |   Performance  |
                 |   Evaluation   |
                 +----------------+
```

### Unit Testing

Unit tests verify individual components and classes.

Examples include:

- Frame tests
- Page-table tests
- Physical-memory tests
- Statistics tests
- Configuration tests
- FIFO tests
- LRU tests
- Clock tests
- Optimal tests

### Integration Testing

Integration tests verify interactions between components.

Examples include:

- MMU + Page Table
- MMU + Physical Memory
- MMU + Pager
- Pager + Replacement Policy
- Simulation + Workload
- Simulation + Statistics

### System Testing

System tests execute complete simulations and verify end-to-end behavior.

### Regression Testing

Regression tests ensure that previously verified behavior remains operational after changes to the implementation.

---

## Memory-Management Model

The EMMUS memory-management model includes the following primary concepts:

### Process

Represents a simulated process that generates memory accesses.

### Virtual Address

Represents an address within a process's virtual address space.

### Page

Represents a fixed-size unit of virtual memory.

### Page Table

Maintains the relationship between virtual pages and physical frames.

### Frame

Represents a fixed-size unit of physical memory.

### Physical Memory

Contains the physical frames available to the simulated system.

### MMU

The Memory Management Unit coordinates virtual-to-physical address translation and memory-access processing.

### Pager

Coordinates page-fault handling and page-replacement operations.

### Replacement Policy

Determines which resident page should be selected when physical memory is full.

---

## Page-Fault Processing

A page fault occurs when a process references a page that is not currently resident in physical memory.

Conceptually, EMMUS handles a page fault as follows:

```text
Page Fault
    |
    v
Is a free frame available?
    |
    +------------------+
    |                  |
   Yes                 No
    |                  |
    v                  v
Allocate Frame    Select Victim
    |                  |
    |                  v
    |             Dirty Page?
    |              /       \
    |            Yes        No
    |             |          |
    |             v          |
    |        Handle Write    |
    |        Back / Evict    |
    |             |          |
    +-------------+----------+
                  |
                  v
             Load Page
                  |
                  v
          Update Page Table
                  |
                  v
        Update Replacement Policy
                  |
                  v
          Continue Execution
```

The exact implementation behavior is defined by the system requirements and detailed software design.

---

## Dirty Pages

A page may be marked dirty when a memory-access operation modifies its contents.

Dirty-page state is important because evicting a modified page may require additional processing before the frame can be reused.

EMMUS will verify:

- Correct dirty-state initialization
- Correct dirty-state updates
- Correct dirty-state preservation
- Correct detection during eviction
- Correct dirty-eviction statistics

---

## Multi-Process Memory Management

EMMUS supports the concept of multiple simulated processes.

The memory-management model must ensure that pages belonging to different processes remain distinguishable even when they use identical virtual page numbers.

For example:

```text
Process 100
    |
    +-- Virtual Page 5
    |
    +-- Virtual Page 6

Process 200
    |
    +-- Virtual Page 5
    |
    +-- Virtual Page 6
```

The two instances of Virtual Page 5 belong to different processes and must therefore remain logically independent.

---

## Workloads

EMMUS is designed to support multiple memory-access workload patterns.

Initial workload categories include:

- Sequential
- Random
- Locality-based
- Repeated-reference
- Mixed

### Sequential Workload

Sequential workloads access pages in an ordered sequence.

Example:

```text
1 2 3 4 5 6 7 8
```

### Random Workload

Random workloads generate memory references using a random process.

A configurable seed should be supported so that a workload can be reproduced.

Example:

```text
4 1 7 2 4 8 3 1 6 5
```

### Locality-Based Workload

Locality workloads repeatedly access a subset of pages.

Example:

```text
1 2 3 2 1 3 2 4 5 4 5 4
```

### Repeated-Reference Workload

Repeated-reference workloads repeatedly access a small set of pages.

Example:

```text
1 2 1 2 1 2 1 2
```

### Mixed Workload

Mixed workloads combine multiple access patterns to create more complex memory-reference behavior.

---

## Reproducibility

Reproducibility is an important project objective.

Where practical, experiments should record:

- EMMUS version or Git commit
- Build configuration
- Configuration file
- Replacement algorithm
- Workload type
- Workload size
- Random seed
- Number of physical frames
- Page size
- Number of processes
- Test environment

When deterministic inputs are used, the same configuration should produce equivalent simulation behavior.

---

## Performance Evaluation

A major objective of EMMUS is to provide a controlled environment for evaluating page-replacement algorithms.

The primary performance metrics include:

- Total memory accesses
- Page faults
- Page-fault rate
- Page replacements
- Dirty-page evictions
- Replacement execution time
- Average replacement execution time
- Total simulation execution time

The algorithms should be compared using equivalent workloads and configurations.

The detailed performance methodology is documented in the Performance Evaluation Report.

---

## Performance Metrics

### Page-Fault Rate

Page-fault rate may be calculated as:

```text
Page-Fault Rate =
    Page Faults / Total Memory Accesses
```

### Average Replacement Time

Average replacement time may be calculated as:

```text
Average Replacement Time =
    Total Replacement Time / Number of Replacements
```

The exact measurement methodology is defined by the performance evaluation process.

---

## Algorithm Comparison

The primary comparison is:

```text
FIFO
 |
 +---- LRU
 |
 +---- Clock
 |
 +---- Optimal
```

The evaluation does not assume that one algorithm is universally superior.

Instead, the project examines trade-offs involving:

- Page-fault behavior
- Computational overhead
- Workload characteristics
- Physical-memory capacity
- Algorithm complexity

Optimal provides a theoretical reference point for comparison with practical algorithms.

---

## Engineering Process

EMMUS is being developed using a structured software and systems engineering lifecycle.

The overall development progression is:

```text
Problem Definition
        |
        v
Requirements
        |
        v
System Architecture
        |
        v
Detailed Software Design
        |
        v
Implementation
        |
        v
Verification
        |
        v
Validation
        |
        v
Performance Evaluation
        |
        v
Documented Results
```

This lifecycle is an important part of the project's portfolio purpose.

The project is intended to demonstrate not only the ability to write software, but also the ability to apply a disciplined engineering process to software development.

---

## Project Documentation

The EMMUS engineering documentation consists of the following primary artifacts:

| Document | Title | Format |
|---|---|---|
| **01** | System Requirements Specification | Word |
| **02** | System Architecture Document | Word |
| **03** | Detailed Software Design Document | Word |
| **04** | Implementation and Development Standards | Word |
| **05** | Implementation and Verification Plan | Word |
| **06** | Requirements Traceability Matrix | Excel |
| **07** | Verification and Validation Report | Word |
| **08** | Performance Evaluation Report | Word |
| **09** | Developer and User Guide | Word |
| **10** | Project README | Markdown |

These artifacts collectively document the project from requirements through design, implementation, verification, validation, operation, and performance evaluation.

---

## Requirements Traceability

EMMUS uses requirements traceability to connect requirements with architecture, design, implementation, and verification.

The general relationship is:

```text
Requirement
     |
     v
Architecture
     |
     v
Detailed Design
     |
     v
Implementation
     |
     v
Verification
     |
     v
Evidence
```

The Requirements Traceability Matrix provides the detailed mapping between requirements and verification activities.

---

## Development Status

> **Current Status:** Initial Project Baseline

The project is being developed incrementally.

The engineering documentation establishes the project baseline before implementation proceeds.

As development progresses, this section will be updated to reflect actual project status.

### Current Status

| Area | Status |
|---|---|
| Requirements | Baseline Established |
| Architecture | Baseline Established |
| Detailed Design | Baseline Established |
| Development Standards | Baseline Established |
| Verification Planning | Baseline Established |
| Requirements Traceability | Baseline Established |
| Implementation | In Development |
| Automated Testing | In Development |
| Verification | Planned |
| Performance Evaluation | Planned |
| Final Validation | Planned |

---

## Planned Development Areas

### Core Memory Management

- [ ] Page model
- [ ] Frame model
- [ ] Physical-memory management
- [ ] Page-table management
- [ ] Address translation

### Memory Management Unit

- [ ] Memory-access processing
- [ ] Page-fault detection
- [ ] Page-fault handling
- [ ] Replacement integration

### Page-Replacement Algorithms

- [ ] FIFO
- [ ] LRU
- [ ] Clock
- [ ] Optimal

### Simulation

- [ ] Process management
- [ ] Workload generation
- [ ] Memory-access execution
- [ ] Statistics collection

### Verification

- [ ] Unit tests
- [ ] Integration tests
- [ ] System tests
- [ ] Regression tests

### Evaluation

- [ ] Algorithm comparison
- [ ] Workload comparison
- [ ] Memory-pressure experiments
- [ ] Performance measurements

---

## Future Enhancements

Potential future enhancements include:

- Additional page-replacement algorithms
- Working-set modeling
- Aging-based replacement
- TLB simulation
- Multi-level page tables
- Additional workload models
- Memory-access visualization
- Interactive simulation
- Additional performance metrics
- Automated benchmark execution
- Expanded GUI functionality

These features are potential future enhancements and are not necessarily part of the initial project scope.

---

## Design Principles

### Separation of Concerns

Each subsystem should have a clearly defined responsibility.

### Encapsulation

Implementation details should remain hidden behind appropriate interfaces.

### Testability

Components should be designed so that their behavior can be independently verified.

### Extensibility

The architecture should allow additional algorithms and workloads to be incorporated with minimal disruption.

### Maintainability

The implementation should favor clear, understandable abstractions rather than unnecessary complexity.

### Determinism

Where practical, deterministic inputs should produce deterministic results.

### Traceability

Requirements should be traceable through architecture, design, implementation, and verification.

---

## Portfolio Objectives

EMMUS is intended to demonstrate both software engineering and systems engineering capabilities.

The project demonstrates experience with:

- Requirements engineering
- Systems architecture
- Software architecture
- Object-oriented programming
- Modern C++ development
- C++23
- Memory-management concepts
- Algorithms and data structures
- Automated testing
- Integration testing
- System testing
- Requirements traceability
- Performance evaluation
- Configuration management
- Build systems
- Version control
- Technical documentation

The project is intentionally structured to demonstrate the complete engineering lifecycle rather than only the final source code.

---

## What the Project Demonstrates

A completed EMMUS project should demonstrate the ability to take a technical problem from concept through implementation and evaluation.

The project lifecycle can be summarized as:

```text
+--------------------------+
|    Problem Definition    |
+------------+-------------+
             |
             v
+--------------------------+
|       Requirements       |
+------------+-------------+
             |
             v
+--------------------------+
|    System Architecture   |
+------------+-------------+
             |
             v
+--------------------------+
|   Detailed Software      |
|          Design          |
+------------+-------------+
             |
             v
+--------------------------+
|       Implementation     |
+------------+-------------+
             |
             v
+--------------------------+
|       Verification       |
+------------+-------------+
             |
             v
+--------------------------+
|        Validation        |
+------------+-------------+
             |
             v
+--------------------------+
|   Performance Evaluation |
+------------+-------------+
             |
             v
+--------------------------+
|     Documented Results   |
+--------------------------+
```

This engineering lifecycle is a central part of the portfolio value of EMMUS.

---

## Limitations

EMMUS is a software simulation and should not be interpreted as a physical implementation of a modern hardware MMU.

The simulator does not attempt to reproduce every detail of:

- Modern CPU memory-management hardware
- Operating-system kernel behavior
- Translation lookaside buffers
- CPU cache hierarchies
- Memory-controller behavior
- Hardware-specific timing

Instead, EMMUS focuses on fundamental memory-management concepts and page-replacement behavior.

Performance measurements generated by EMMUS represent simulator and algorithm behavior. They should not be interpreted as measurements of actual hardware MMU performance.

---

## Quality Goals

The completed EMMUS project should strive to provide:

- Correct behavior
- Repeatable simulations
- Automated verification
- Clear architecture
- Maintainable source code
- Meaningful error handling
- Reproducible experiments
- Complete requirements traceability
- Professional documentation

---

## Getting Started

A developer or evaluator encountering the project for the first time should begin with this README.

The recommended documentation sequence is:

1. `README.md`
2. System Requirements Specification
3. System Architecture Document
4. Detailed Software Design Document
5. Implementation and Development Standards
6. Implementation and Verification Plan
7. Requirements Traceability Matrix
8. Verification and Validation Report
9. Performance Evaluation Report
10. Developer and User Guide

The README provides the high-level project overview, while the engineering documents provide progressively greater technical detail.

---

## Contributing

Development changes should follow the engineering practices established by the project.

Before modifying functionality, developers should:

1. Review the applicable requirements.
2. Review the relevant architecture.
3. Review the detailed design.
4. Implement the change.
5. Add or update automated tests.
6. Build the project.
7. Run the test suite.
8. Verify that existing functionality remains operational.
9. Update traceability where necessary.
10. Update documentation where necessary.

---

## Version Control

Git is used for source-code version control.

Commits should:

- Represent coherent changes.
- Use descriptive commit messages.
- Avoid unrelated modifications.
- Include appropriate tests with functional changes.
- Preserve a buildable project where practical.

Significant architectural changes should be documented.

---

## Continuous Integration

Where configured, continuous integration should automatically:

1. Check out the repository.
2. Configure the build.
3. Compile the project.
4. Compile the test suite.
5. Execute automated tests.
6. Report failures.

CI provides an additional mechanism for detecting build and regression problems.

---

## License

A project license will be selected before the initial public release.

**Current Status:** TBD

Until a license is explicitly included in the repository, users should not assume that the source code is freely redistributable.

---

## Author

**Author:** EMMUS Project

Additional author and contact information may be added to the repository as appropriate.

---

## Project Status

**Current Project Status:** Initial Project Baseline

**Implementation:** In Development

**Automated Testing:** In Development

**Verification:** Planned

**Validation:** Planned

**Performance Evaluation:** Planned

**Documentation:** Engineering Baseline Established

---

## Conclusion

The **Enhanced Memory Management Unit Simulator (EMMUS)** is a software and systems engineering portfolio project designed to demonstrate the complete development lifecycle of a non-trivial C++ application.

The project combines a technically meaningful problem domain—computer memory management—with professional engineering practices including:

- Requirements engineering
- System architecture
- Detailed software design
- Modular implementation
- Automated verification
- Requirements traceability
- Performance evaluation
- Configuration management
- Version control
- Technical documentation

The initial implementation focuses on virtual memory, physical memory, page tables, frames, address translation, page faults, page replacement, dirty pages, workloads, and multiple replacement algorithms.

The completed project will provide not only a functioning memory-management simulator, but also a documented body of engineering evidence demonstrating how the system was:

**conceived → specified → designed → implemented → tested → verified → validated → evaluated**

This combination of software implementation and engineering documentation is a primary objective of the EMMUS portfolio project.