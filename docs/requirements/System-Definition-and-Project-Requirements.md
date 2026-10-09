# EMMUS 2.0

## System Definition and Project Requirements

**Document Phase:** Phase 1 — Project Definition  
**Project:** Enhanced Memory Management Unit Simulator (EMMUS)  
**Project Type:** Software / Systems Engineering Portfolio Project  
**Programming Language:** C++23  
**Build System:** CMake  
**Testing Framework:** GoogleTest  
**Source Control:** Git  
**Continuous Integration:** GitHub Actions


# 1. Introduction


### 1.1 Purpose

EMMUS (Enhanced Memory Management Unit Simulator) is a software-based simulator designed to model the operation of a memory-management subsystem.

The system will provide a configurable environment for demonstrating and analyzing:

- Virtual memory
- Physical memory
- Pages and frames
- Page tables
- Virtual-to-physical address translation
- Page faults
- Page loading and eviction
- Page-replacement algorithms
- Process memory-access behavior
- Memory-management statistics
EMMUS is being developed primarily as a software and systems engineering portfolio project. The project is intended to demonstrate the ability to take a systems problem from requirements through architecture, implementation, verification, performance analysis, documentation, and release.

The project shall prioritize engineering quality, maintainability, testability, and demonstrable technical capability rather than maximizing the number of features or documentation artifacts.


### 1.2 Project Objectives

The primary objectives of EMMUS are to:

1. Develop a technical sound simulation of virtual-memory management.
1. Demonstrate modern C++23 software development.
1. Demonstrate software and systems architecture skills.
1. Demonstrate object-oriented and interface-based design.
1. Demonstrate the appropriate application of software design patterns.
1. Demonstrate algorithms and data-structure implementation.
1. Demonstrate automated unit and integration testing.
1. Demonstrate build-system and dependency management using CMake.
1. Demonstrate continuous integration.
1. Demonstrate logging, diagnostics, and performance measurement.
1. Provide meaningful simulation results that can be analyzed and compared.
1. Produce a professional Git repository suitable for presentation to prospective employers.
The project shall demonstrate the complete engineering lifecycle:

Problem

```text
↓
```
Requirements

```text
↓
```
Architecture

```text
↓
```
Detailed Design

```text
↓
```
Implementation

```text
↓
```
Verification

```text
↓
```
Performance Analysis

```text
↓
```
Documentation

```text
↓
```
Release


### 1.3 Project Goals

The primary goal of EMMUS is to create a realistic yet manageable simulation environment that demonstrates how memory-management mechanisms operate together.

The project shall balance three objectives:

Technical Correctness

The simulator should accurately model the memory-management behavior defined by its requirements.

Engineering Quality

The software should exhibit sound architecture, modularity, testability, maintainability, and reproducibility.

Portfolio Value

The completed project should provide clear evidence of practical software and systems-engineering capabilities to a technical reviewer.


# 2. System Overview


### 2.1 System Objective

The primary objective of the system is:

To provide a configurable simulation environment that models virtual-memory management and allows users to observe, analyze, and compare memory-management behavior under different workloads and page-replacement policies.

All major EMMUS features should support this objective.

Features that do not contribute meaningfully to simulation, analysis, or demonstration of memory-management behavior should be evaluated carefully before being added to the core system.


### 2.2 Intended Users

EMMUS will support three primary audiences.


#### 2.2.1 Technical Reviewer

The technical reviewer may be:

- A software engineer
- A systems engineer
- A C++ developer
- A hiring manager
- A technical interviewer
The reviewer should be able to understand the purpose and architecture of EMMUS and run a meaningful simulation without extensive explanation.


#### 2.2.2 Student or Learner

EMMUS should provide a useful environment for learning about:

- Virtual memory
- Address translation
- Page tables
- Page faults
- Physical memory
- Frames
- Page replacement
- Memory-access locality
- Memory-management performance

#### 2.2.3 Developer

A developer should be able to:

- Build the project
- Run the automated tests
- Run simulations
- Understand the architecture
- Add new workloads
- Add new page-replacement algorithms
- Modify existing components
- Extend the system without unnecessary modification of unrelated components

### 2.3 System Scope

The initial EMMUS system will model the following areas.


#### 2.3.1 Virtual Memory

The system will support:

- Virtual address spaces
- Pages
- Virtual addresses
- Page identifiers
- Address translation

#### 2.3.2 Physical Memory

The system will support:

- Physical memory
- Frames
- Frame allocation
- Frame ownership
- Page loading
- Page eviction

#### 2.3.3 Page Tables

The system will support:

- Page-table entries
- Virtual-page-to-physical-frame mappings
- Page residency information
- Page access state
- Page modification state

#### 2.3.4 Page Faults

The system will:

- Detect accesses to non-resident pages
- Generate page-fault events
- Locate an available frame when possible
- Invoke page replacement when necessary
- Load the required page
- Update the appropriate page-table state

#### 2.3.5 Page Replacement

The initial implementation will support:

- FIFO
- LRU
- Clock
- Optimal
The architecture shall allow additional page-replacement algorithms to be added without requiring unnecessary changes to unrelated memory-management components.


#### 2.3.6 Processes

The simulator will support:

- Multiple simulated processes
- Process identification
- Process-specific virtual address spaces
- Process memory-access workloads

#### 2.3.7 Workloads

The simulator will support controlled memory-access workloads.

Workloads may eventually represent different access patterns, including:

- Sequential access
- Random access
- Locality-based access
- Repeated access
- Multi-process access

#### 2.3.8 Statistics

The simulator will collect statistics related to:

- Page faults
- Page replacements
- Dirty-page evictions
- Memory accesses
- Page-fault rate
- Algorithm execution time
- Overall simulation performance

#### 2.3.9 Observability

The system will provide mechanisms for observing significant simulation events through:

- Structured logging
- Simulation statistics
- Simulation summaries
- Performance measurements

# 3. Functional Requirements

The following requirements establish the initial functional baseline for EMMUS.


### 3.1 Memory Management Requirements

FR-001 — The system shall represent virtual memory as a collection of pages.

FR-002 — The system shall represent physical memory as a collection of frames.

FR-003 — The system shall maintain mappings between virtual pages and physical frames.

FR-004 — The system shall translate valid virtual addresses into physical addresses.

FR-005 — The system shall detect accesses to non-resident pages.

FR-006 — The system shall generate a page-fault event when a required page is not resident in physical memory.

FR-007 — The system shall load required pages into available physical frames.

FR-008 — The system shall evict a resident page when no suitable free frame is available.


### 3.2 Page-Replacement Requirements

FR-009 — The system shall provide a common interface for page-replacement algorithms.

FR-010 — The system shall implement FIFO page replacement.

FR-011 — The system shall implement LRU page replacement.

FR-012 — The system shall implement Clock page replacement.

FR-013 — The system shall implement Optimal page replacement.

FR-014 — The system shall allow the page-replacement policy to be selected through configuration or runtime selection.


### 3.3 Process Requirements

FR-015 — The system shall support multiple simulated processes.

FR-016 — Each simulated process shall have an identifiable virtual address space.

FR-017 — The system shall support process memory-access workloads.


### 3.4 Statistics Requirements

FR-018 — The system shall record page-fault statistics.

FR-019 — The system shall record page-replacement statistics.

FR-020 — The system shall record dirty-page eviction statistics.

FR-021 — The system shall record simulation performance statistics.


### 3.5 Configuration Requirements

FR-022 — The system shall allow simulation parameters to be configured.

At minimum, configurable parameters shall eventually include:

- Physical-memory size
- Page size
- Number of processes
- Page-replacement algorithm
- Workload
- Simulation length

### 3.6 Observability Requirements

FR-023 — The system shall provide structured logging of significant memory-management events.

FR-024 — The system shall provide a mechanism for inspecting simulation results.


# 4. Non-Functional Requirements


### 4.1 Maintainability

NFR-001 — The system shall use clearly defined interfaces and modular components so that individual subsystems can be modified independently.


### 4.2 Extensibility

NFR-002 — The architecture shall allow new page-replacement algorithms to be added without requiring unnecessary modification of unrelated memory-management components.


### 4.3 Testability

NFR-003 — Core system components shall be independently testable.


### 4.4 Reproducibility

NFR-004 — A developer shall be able to obtain a clean source tree and reproduce the build and automated test suite using documented procedures.


### 4.5 Portability

NFR-005 — The core simulation system should minimize unnecessary platform-specific dependencies.


### 4.6 Performance

NFR-006 — The simulator shall be capable of processing workloads large enough to support meaningful comparisons between page-replacement algorithms.


### 4.7 Reliability

NFR-007 — Invalid configuration values and invalid memory accesses shall be detected and handled predictably.


### 4.8 Observability

NFR-008 — Significant simulation events and performance characteristics shall be observable through logging and simulation statistics.


### 4.9 Documentation

NFR-009 — A developer unfamiliar with the project shall be able to understand its architecture and build the project using the provided documentation.


### 4.10 Automated Verification

NFR-010 — The project shall maintain an automated test suite that can be executed as part of the normal development and build process.


# 5. Technology Baseline

The initial technology baseline for EMMUS is:

| Area | Technology |
| --- | --- |
| Programming Language | C++23 |
| Build System | CMake |
| Unit Testing | GoogleTest |
| Source Control | Git |
| Continuous Integration | GitHub Actions |

Additional technologies may be introduced when justified by system requirements or architectural needs.

Dependencies should not be introduced solely for convenience when an existing C++ standard-library solution is sufficient.


# 6. Graphical User Interface

A graphical user interface may be provided as part of EMMUS, but the GUI shall not define or control the architecture of the core simulation system.

The core simulation engine shall remain independently buildable and testable.

The conceptual relationship should be:

```text
┌───────────────────┐
│       GUI         │
└─────────┬─────────┘
│
▼
┌───────────────────┐
│   EMMUS Core      │
│ Simulation Engine │
└─────────┬─────────┘
│
▼
┌───────────────────┐
│ Memory Management │
│      System       │
└───────────────────┘
```
The final GUI technology and architecture will be determined during subsequent architecture activities.


# 7. System Boundary

At a conceptual level, EMMUS will contain the following major subsystems:

```text
┌──────────────────────────────────────────────────┐
│                      EMMUS                       │
│                                                  │
│  ┌─────────────┐       ┌────────────────────┐   │
│  │ User / CLI  │──────▶│ Simulation Engine  │   │
│  └─────────────┘       └─────────┬──────────┘   │
│                                  │              │
│                         ┌────────▼─────────┐    │
│                         │ Memory Manager   │    │
│                         └────────┬─────────┘    │
│                                  │              │
│               ┌──────────────────┼───────────┐  │
│               │                  │           │  │
│        ┌──────▼──────┐    ┌──────▼──────┐    │  │
│        │ Page Tables │    │  Physical    │    │  │
│        │             │    │  Memory      │    │  │
│        └─────────────┘    └──────┬───────┘    │  │
│                                  │            │  │
│                         ┌────────▼────────┐   │  │
│                         │ Page Replacement│   │  │
│                         └─────────────────┘   │  │
│                                               │  │
│               ┌───────────────────────────┐   │  │
│               │ Statistics / Logging      │   │  │
│               └───────────────────────────┘   │  │
│                                               │  │
└──────────────────────────────────────────────────┘
```
These boundaries are conceptual only. The formal architecture will be established during Phase 2.


# 8. Initial Simulation Model

EMMUS shall support a simulation model in which a process performs a virtual-memory access.

A conceptual access sequence is:

Process

```text
│
▼
```
Virtual Address

```text
│
▼
```
Page Table Lookup

```text
│
▼
```
Is Page Resident?

```text
│
├────────────── Yes ──────────────┐
│                                 │
│                                 ▼
│                         Physical Address
│                                 │
│                                 ▼
│                           Memory Access
│
└────────────── No ───────────────┐
│
▼
```
Page Fault

```text
│
▼
```
Find Free Frame

```text
│
┌──────┴──────┐
│             │
```
Available      None

```text
│             │
│             ▼
│      Page Replacement
│             │
└──────┬──────┘
▼
```
Load Page

```text
│
▼
```
Update Page Table

```text
│
▼
```
Memory Access

The final implementation details will be determined during architecture and detailed-design activities.


# 9. Success Criteria

EMMUS 2.0 shall be considered successful when a person unfamiliar with the project can:

1. Clone the repository.
1. Follow the documented build procedure.
1. Build the project successfully.
1. Execute the automated test suite.
1. Run a memory-management simulation.
1. Select a page-replacement algorithm.
1. Execute a defined workload.
1. Observe significant memory-management events.
1. Obtain meaningful simulation statistics.
1. Compare the behavior of multiple page-replacement algorithms.
1. Understand the high-level architecture from the project documentation.
1. Add a new page-replacement algorithm without redesigning the core memory-management system.

# 10. Portfolio Demonstration

EMMUS shall provide at least one reproducible demonstration scenario suitable for presentation as a portfolio project.

An example demonstration scenario may include:

Physical Memory:       4 frames

Page Size:             4 KB

Processes:             2

Workload:              Locality-based

Replacement Policy:    LRU

Memory Accesses:       10,000

The demonstration should produce results such as:

Simulation Results

```text
─────────────────────────────
```
Algorithm:              LRU

Memory Frames:          4

Memory Accesses:        10,000

Page Faults:             XXXX

Page Replacements:       XXXX

Dirty Evictions:          XXX

Page Fault Rate:        XX.XX%

The exact simulation parameters and output format will be established during later project phases.

The purpose of the demonstration is to provide a concise, reproducible example of the system's capabilities.


# 11. Documentation Strategy

EMMUS will intentionally avoid excessive documentation.

The project should maintain a small set of authoritative engineering documents rather than creating separate documents for every subsystem or feature.

The initial documentation set is expected to include approximately:

1. EMMUS System Definition and Project Requirements
1. EMMUS System Architecture and Design
1. EMMUS Verification and Validation
1. EMMUS Developer and Build Guide
1. EMMUS User Guide
1. EMMUS Requirements Traceability
A separate project-management document may be maintained if necessary.

Additional lightweight artifacts may include:

- README.md
- CHANGELOG.md
- Architecture Decision Records (ADRs)
- Test reports
- Release information
- Issue tracking
Related information should be consolidated into the appropriate authoritative document.

The existence of a subsystem or feature shall not, by itself, justify creation of a separate document.


# 12. Deferred Design Decisions

The following decisions shall not be finalized during Phase 1:

- Detailed class hierarchy
- Exact interfaces
- Namespace structure
- Directory structure
- Object ownership and lifetime model
- Smart-pointer strategy
- Threading model
- GUI architecture
- CMake target structure
- Detailed page-table implementation
- Detailed statistics implementation
- Exact logging implementation
- Design-pattern selection
- Configuration-file format
- Persistence strategy
These decisions will be addressed during subsequent architecture and design activities.


# 13. Phase 1 Baseline

Phase 1 establishes EMMUS as:

A configurable virtual-memory and Memory Management Unit simulator designed to demonstrate memory-management concepts while providing a substantial example of professional C++ and systems-engineering practice.

The initial functional scope includes:

- Virtual memory
- Physical memory
- Pages
- Frames
- Page tables
- Address translation
- Page faults
- Page loading
- Page eviction
- FIFO page replacement
- LRU page replacement
- Clock page replacement
- Optimal page replacement
- Multiple simulated processes
- Memory-access workloads
- Simulation statistics
- Logging and observability
The initial engineering technology stack is:

C++23 + CMake + GoogleTest + Git + GitHub Actions

The project will follow the engineering lifecycle:

Requirements → Architecture → Design → Implementation → Verification → Analysis → Release

The project will maintain a concise documentation set and avoid unnecessary documentation overhead.

End of Document
