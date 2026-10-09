**EMMUS 2.0**

**System Architecture and Design**

**Document Phase:** Phase 2 — System Architecture  
**Project:** Enhanced Memory Management Unit Simulator (EMMUS)  
**Project Type:** Software / Systems Engineering Portfolio Project  
**Programming Language:** C++23  
**Build System:** CMake  
**Testing Framework:** GoogleTest  
**Source Control:** Git  
**Continuous Integration:** GitHub Actions

# 1. Introduction

## 1.1 Purpose

This document defines the high-level system architecture and architectural design of the Enhanced Memory Management Unit Simulator (EMMUS).

The purpose of this document is to transform the requirements established during Phase 1 into a coherent software architecture that: - Separates system responsibilities. - Defines major subsystems and their boundaries. - Establishes dependencies between components. - Defines important interfaces. - Supports extensibility. - Supports independent testing. - Minimizes unnecessary coupling. - Provides a foundation for detailed software design and implementation.

This document establishes **what the major software components are responsible for and how they interact**.

Detailed class design, implementation details, algorithms, and source-code organization will be addressed during subsequent development activities.

# 2. Architectural Goals

The EMMUS architecture shall support the following goals.

## 2.1 Separation of Concerns

Each subsystem shall have a clearly defined responsibility.

A component should not perform work that belongs to another subsystem unless that responsibility is explicitly part of its design.

## 2.2 Modularity

Major subsystems shall be independently understandable, testable, and replaceable where practical.

## 2.3 Extensibility

The architecture shall permit new page-replacement algorithms and workloads to be introduced without requiring significant changes to unrelated components.

## 2.4 Testability

Core simulation components shall be usable without the graphical user interface.

## 2.5 Dependency Control

High-level simulation behavior should depend on stable abstractions rather than concrete implementations wherever practical.

## 2.6 Reproducibility

The architecture shall support deterministic simulation execution when deterministic configuration and workloads are provided.

## 2.7 Observability

The architecture shall provide appropriate mechanisms for capturing simulation events, statistics, and performance information.

## 2.8 Portfolio Quality

The architecture should clearly demonstrate professional software-engineering practices without introducing unnecessary complexity solely for architectural sophistication.

# 3. Architectural Principles

The following principles will guide EMMUS architecture and implementation.

## 3.1 Single Responsibility

A component should have one primary responsibility and one reason to change.

For example, physical-memory management should not also be responsible for selecting page-replacement algorithms.

## 3.2 Dependency Inversion

Core system components should depend on stable abstractions where doing so provides meaningful architectural benefit.

## 3.3 Encapsulation

Subsystems shall control their internal state and expose only the operations required by their clients.

## 3.4 Composition Over Inheritance

Inheritance shall be used primarily where polymorphic behavior is required.

Composition should be preferred for assembling system behavior.

## 3.5 Interface-Based Extensibility

Variable behavior, particularly page-replacement policies, shall be represented through explicit interfaces.

## 3.6 Deterministic Core

The core simulation engine should avoid unnecessary nondeterminism.

Given the same configuration, workload, and initial state, the simulator should produce equivalent results.

## 3.7 Core Independence

The core simulation system shall not depend on GUI-specific functionality.

## 3.8 Explicit Ownership

Ownership and lifetime of important system objects shall be clearly defined.

## 3.9 Fail Predictably

Invalid operations and invalid configuration shall result in predictable error handling rather than undefined behavior.

## 3.10 Measure Before Optimizing

Performance optimizations shall be driven by measurements rather than premature assumptions.

# 4. System Context

At the highest level, EMMUS consists of an application layer, a simulation layer, and a memory-management subsystem.

┌──────────────────────┐

│ User │

└──────────┬───────────┘

│

┌──────────────┴──────────────┐

│ │

▼ ▼

┌─────────────┐ ┌─────────────┐

│ CLI │ │ GUI │

└──────┬──────┘ └──────┬──────┘

│ │

└────────────┬───────────────┘

│

▼

┌────────────────────────┐

│ Simulation Engine │

└────────────┬───────────┘

│

▼

┌────────────────────────┐

│ Memory Management │

│ Subsystem │

└────────────┬───────────┘

│

┌───────────────────┼───────────────────┐

│ │ │

▼ ▼ ▼

┌────────────┐ ┌────────────┐ ┌──────────────┐

│ Page Tables│ │ Physical │ │ Page │

│ │ │ Memory │ │ Replacement │

└────────────┘ └────────────┘ └──────────────┘

│

▼

┌─────────────────┐

│ Statistics / │

│ Observability │

└─────────────────┘

The GUI and CLI are external interfaces to the simulation system rather than components that own the underlying memory-management logic.

# 5. Architectural Layers

EMMUS shall use a layered architecture.

## 5.1 Presentation Layer

The presentation layer provides user-facing interfaces.

Initial interfaces may include: - Command-line interface - Graphical user interface

The presentation layer shall translate user input into requests to the application or simulation layer.

It shall not implement memory-management algorithms.

## 5.2 Application Layer

The application layer coordinates simulations.

Responsibilities include: - Loading configuration. - Creating simulation components. - Selecting workloads. - Selecting page-replacement policies. - Starting and stopping simulations. - Coordinating simulation execution. - Collecting simulation results. - Providing results to presentation components.

The application layer should coordinate the system without becoming responsible for low-level memory-management behavior.

## 5.3 Simulation Layer

The simulation layer represents the execution of processes and memory-access workloads.

Responsibilities include: - Simulated processes. - Workload execution. - Memory-access generation. - Simulation timing or sequencing. - Issuing virtual-memory access requests. - Maintaining simulation state.

The simulation layer should interact with memory management through defined interfaces.

## 5.4 Memory Management Layer

The memory-management layer contains the core MMU and memory-management behavior.

Responsibilities include: - Virtual address translation. - Page-table interaction. - Page-fault detection. - Physical-frame management. - Page loading. - Page eviction. - Page replacement. - Page-state management.

This layer is the technical center of EMMUS.

## 5.5 Infrastructure Layer

The infrastructure layer provides cross-cutting services such as: - Logging - Configuration - Performance measurement - Error reporting - File I/O where required

Infrastructure services should remain independent of the core memory-management algorithms wherever practical.

# 6. Major Subsystems

The EMMUS architecture is divided into the following major subsystems.

EMMUS

│

├── Presentation

│ ├── CLI

│ └── GUI

│

├── Application

│ └── Simulation Coordinator

│

├── Simulation

│ ├── Process Management

│ ├── Workload Management

│ └── Simulation Execution

│

├── Memory Management

│ ├── MMU

│ ├── Virtual Memory

│ ├── Page Tables

│ ├── Physical Memory

│ └── Page Replacement

│

├── Analysis

│ └── Statistics

│

└── Infrastructure

├── Configuration

├── Logging

└── Timing

# 7. Presentation Subsystem

## 7.1 Responsibilities

The presentation subsystem shall: - Accept user input. - Display simulation configuration. - Start simulations. - Display simulation progress where appropriate. - Display simulation results. - Provide access to relevant system information.

## 7.2 Architectural Constraint

Presentation components shall not directly manipulate: - Page tables - Frames - Page-replacement queues - Process memory state - Physical-memory state

Instead, they shall communicate with the application or simulation layer.

## 7.3 CLI

The command-line interface should provide a lightweight way to: - Run simulations. - Specify configuration. - Select algorithms. - Select workloads. - Display results.

The CLI should also serve as a convenient interface for automated testing and CI demonstrations.

## 7.4 GUI

A GUI may provide: - Simulation configuration. - Real-time or post-simulation visualization. - Memory-state visualization. - Page-table visualization. - Algorithm comparison. - Statistics visualization.

The GUI shall remain optional to the operation of the core simulation engine.

# 8. Application Subsystem

## 8.1 Simulation Coordinator

The application layer shall contain a central simulation-coordination responsibility.

The coordinator shall: 1. Receive or load simulation configuration. 2. Validate configuration. 3. Construct required components. 4. Select the requested workload. 5. Select the requested page-replacement policy. 6. Initialize simulation state. 7. Execute the simulation. 8. Collect results. 9. Return or publish the results.

The coordinator shall not implement the internal behavior of the MMU or page-replacement algorithms.

# 9. Simulation Subsystem

## 9.1 Process Management

The process-management subsystem shall represent simulated processes.

A process should have: - A unique process identifier. - A virtual address space. - Associated memory-access behavior. - Appropriate process state.

The exact process model will be established during detailed design.

## 9.2 Workload Management

A workload represents a sequence or generation strategy for memory accesses.

The architecture should support multiple workload implementations through an abstraction.

Conceptually:

┌───────────────────────┐

│ IWorkload │

└───────────┬───────────┘

│

┌─────────────┼──────────────┐

│ │ │

▼ ▼ ▼

Sequential Random Locality-Based

Additional workloads should be addable without modifying the simulation engine.

## 9.3 Simulation Execution

The simulation executor shall: - Obtain memory accesses from processes/workloads. - Submit accesses to the memory-management subsystem. - Track simulation progress. - Collect or propagate simulation events. - Stop according to the configured simulation criteria.

# 10. Memory Management Subsystem

The memory-management subsystem is the core of EMMUS.

It shall contain the major responsibilities required to model virtual-memory behavior.

┌──────────────────────────────────────────┐

│ Memory Management Subsystem │

│ │

│ ┌────────────┐ ┌───────────────┐ │

│ │ MMU │──────▶│ Page Tables │ │

│ └─────┬──────┘ └───────────────┘ │

│ │ │

│ ▼ │

│ ┌──────────────┐ │

│ │ Page │ │

│ │ Fault │ │

│ │ Handling │ │

│ └──────┬───────┘ │

│ │ │

│ ▼ │

│ ┌──────────────┐ ┌────────────────┐ │

│ │ Physical │───▶│ Page │ │

│ │ Memory │ │ Replacement │ │

│ └──────────────┘ └────────────────┘ │

│ │

└──────────────────────────────────────────┘

# 11. Memory Management Unit

## 11.1 Responsibilities

The MMU shall serve as the primary interface for virtual-memory accesses.

A conceptual MMU operation is:

Virtual Memory Access

│

▼

MMU

│

▼

Address Translation

│

├───────────────┐

│ │

Hit Fault

│ │

│ ▼

│ Page-Fault Handler

│ │

│ ▼

│ Page Replacement

│ │

│ ▼

└──────────► Physical Access

## 11.2 MMU Responsibilities

The MMU shall: - Accept a virtual-memory access request. - Determine the relevant process and virtual page. - Consult page-table state. - Determine whether the page is resident. - Generate a page fault when required. - Coordinate page loading. - Coordinate page replacement when required. - Produce the resulting physical-memory access or appropriate failure result. - Update relevant access state.

The MMU shall coordinate these operations rather than directly implementing every subsystem’s internal behavior.

# 12. Virtual Memory

The virtual-memory model shall represent the logical address space visible to each simulated process.

The architecture shall distinguish between: - Virtual addresses - Virtual pages - Page offsets - Process address spaces - Physical frames

The translation model shall conceptually follow:

Virtual Address

│

├───────────────┐

│ │

▼ ▼

Virtual Page Page Offset

│

▼

Page Table

│

▼

Physical Frame

│

└───────────────┐

▼

Physical Address

The exact address-width and page-size model will be established during detailed design.

# 13. Page Table Subsystem

## 13.1 Responsibilities

The page-table subsystem shall maintain mappings between virtual pages and physical frames.

A page-table entry may conceptually contain information such as:

Page Table Entry

────────────────────────

Virtual Page

Physical Frame

Present / Resident

Dirty

Referenced

Permissions

The exact fields shall be determined during detailed design.

## 13.2 Page Table Responsibilities

The subsystem shall provide operations for: - Looking up a virtual page. - Determining residency. - Obtaining a mapped frame. - Creating mappings. - Removing mappings. - Updating page state. - Updating access state.

The page-table implementation shall encapsulate its internal data structure.

# 14. Physical Memory Subsystem

## 14.1 Responsibilities

The physical-memory subsystem shall represent the simulator’s available physical memory.

It shall manage: - Frames. - Frame allocation. - Frame occupancy. - Page-to-frame relationships. - Frame release. - Physical-memory capacity.

## 14.2 Frame

A frame represents a unit of physical memory capable of holding one page.

The frame abstraction should maintain the state required to determine: - Whether the frame is available. - Which process owns the resident page. - Which virtual page occupies the frame. - Relevant state required by replacement policies.

The exact representation will be established during detailed design.

# 15. Page-Replacement Subsystem

Page replacement is one of the primary extensibility points of EMMUS.

The architecture shall use a common abstraction for replacement policies.

Conceptually:

┌──────────────────────────┐

│ IPageReplacementPolicy │

└─────────────┬────────────┘

│

┌────────────────────┼────────────────────┐

│ │ │

▼ ▼ ▼

FIFO LRU Clock

│

└────────────────────┐

▼

Optimal

## 15.1 Common Interface

The page-replacement abstraction shall provide operations sufficient for the memory-management subsystem to: - Notify the policy when a page is loaded. - Notify the policy when a page is accessed. - Notify the policy when a page is removed. - Request a victim frame or page. - Obtain policy-specific statistics where appropriate.

The exact interface will be finalized during detailed software design.

## 15.2 FIFO

FIFO shall select pages according to their residency order.

The implementation should maintain the ordering information necessary to identify the oldest eligible page.

## 15.3 LRU

LRU shall select the page that has remained unused for the longest period according to the simulator’s access history.

## 15.4 Clock

Clock shall use a reference-bit-based approximation of LRU behavior.

The implementation shall maintain the state required to perform the clock-hand search.

## 15.5 Optimal

Optimal replacement shall select the page whose next required use occurs furthest in the future, or which is not used again.

Because this requires knowledge of future memory accesses, the architecture shall allow the Optimal policy to receive or access the required workload information without creating inappropriate dependencies in other replacement algorithms.

# 16. Page-Replacement Policy Selection

Page-replacement algorithms shall be selected through an abstraction rather than through hard-coded conditional logic distributed throughout the memory-management subsystem.

Conceptually:

Configuration

│

▼

Policy Selection

│

▼

Policy Factory / Registry

│

├──── FIFO

├──── LRU

├──── Clock

└──── Optimal

The final implementation mechanism may use a factory, registry, or another suitable pattern.

The architectural requirement is that policy selection remain centralized and extensible.

# 17. Page-Fault Handling

Page-fault handling shall follow a defined sequence.

A conceptual page-fault process is:

Virtual Access

│

▼

Page Table Lookup

│

▼

Page Resident?

┌──┴──┐

Yes No

│ │

│ ▼

│ Page Fault

│ │

│ ▼

│ Free Frame?

│ ┌──┴──┐

│ Yes No

│ │ │

│ │ ▼

│ │ Select Victim

│ │ │

│ │ ▼

│ │ Evict Page

│ │ │

└───┴──────┘

│

▼

Load Page

│

▼

Update Mapping

│

▼

Complete Access

The page-fault handling mechanism shall be responsible for coordinating the required operations while allowing each subsystem to retain ownership of its internal responsibilities.

# 18. Dirty Page Handling

A page may be marked dirty when it has been modified.

When a dirty page is selected for eviction, the memory-management system shall: 1. Identify the page as dirty. 2. Record the dirty eviction. 3. Perform the appropriate simulated write-back behavior. 4. Remove or update the page-table mapping. 5. Reuse the frame for the required page.

The simulator does not need to model real disk hardware unless a future requirement explicitly introduces such behavior.

The initial system should model the **logical consequences** of dirty-page eviction rather than reproduce a complete operating-system storage subsystem.

# 19. Statistics and Analysis Subsystem

The statistics subsystem shall collect measurable information about simulation behavior.

Potential metrics include: - Total memory accesses - Page faults - Page-fault rate - Page replacements - Dirty-page evictions - Successful translations - Algorithm execution time - Total simulation execution time - Average replacement decision time

Statistics shall be collected without requiring the core algorithms to know how results are ultimately displayed.

Conceptually:

Simulation

│

├───────────────┐

│ │

▼ ▼

Memory Events Timing

│ │

└───────┬───────┘

▼

Statistics

│

┌─────┴─────┐

▼ ▼

Console GUI

# 20. Logging and Observability

Logging shall be treated as a cross-cutting infrastructure service.

The core system may generate events such as: - Process creation - Memory access - Page hit - Page fault - Page load - Page replacement - Dirty-page eviction - Simulation completion - Configuration errors

Logging should support multiple severity levels, such as: - Trace - Debug - Information - Warning - Error

The exact logging framework and API will be established during implementation planning.

Core components should not become dependent on a particular user interface merely to report events.

# 21. Configuration Subsystem

Configuration shall provide simulation parameters to the application layer.

Configuration may include:

Simulation Configuration

──────────────────────────────

Page Size

Physical Memory Size

Number of Processes

Replacement Algorithm

Workload

Number of Memory Accesses

Logging Level

Random Seed

Configuration validation shall occur before simulation execution begins.

Invalid configurations shall produce clear diagnostic information.

# 22. Dependency Architecture

The intended dependency direction is:

Presentation

│

▼

Application

│

▼

Simulation

│

▼

Memory Management

│

├───────────────┐

▼ ▼

Page Tables Physical Memory

│ │

└───────┬───────┘

▼

Page Replacement

Infrastructure services

▲

│

Used through appropriate abstractions

The GUI shall not become a dependency of the simulation engine.

Likewise: - Page-replacement algorithms should not depend on the GUI. - Physical memory should not depend on presentation components. - Page tables should not depend on the CLI. - Core memory-management components should not depend on application-specific output formatting.

# 23. Interface Architecture

> The following interfaces are anticipated as important architectural boundaries.

## 23.1 Memory Access Interface

> Provides a standardized mechanism for submitting virtual-memory accesses to the MMU.

## 23.2 Page Replacement Interface

> Provides the common abstraction implemented by replacement policies.

## 23.3 Workload Interface

> Provides a standardized mechanism for generating memory-access sequences.

## 23.4 Statistics Interface

> Provides a mechanism for recording or retrieving simulation measurements.

## 23.5 Logging Interface

> Provides an abstraction for diagnostic and simulation event reporting.
>
> The exact names, signatures, and ownership models will be determined during detailed software design.

# 24. Data Flow

A normal memory access shall follow approximately this flow:

Process

│

▼

Workload

│

▼

Virtual Memory Access

│

▼

Simulation Engine

│

▼

MMU

│

▼

Page Table

│

├────────── Page Resident ──────────┐

│ │

│ ▼

│ Physical Memory

│ │

│ ▼

│ Statistics

│

└────────── Page Fault ────────────┐

│

▼

Page-Fault Handler

│

▼

Physical Memory

│

┌───────┴────────┐

│ │

Free Frame No Free Frame

│ │

│ ▼

│ Replacement Policy

│ │

└────────┬───────┘

▼

Load Page

│

▼

Page Table

│

▼

Physical Memory

│

▼

Statistics

# 25. Error Handling Architecture

Errors shall be categorized according to their nature.

## 25.1 Configuration Errors

Examples include: - Invalid page size - Invalid physical-memory size - Unsupported replacement algorithm - Invalid workload configuration

These should be detected before simulation execution.

## 25.2 Simulation Errors

Examples include: - Invalid process identifier - Invalid virtual address - Invalid memory access - Invalid simulation state

These should be handled through defined error mechanisms.

## 25.3 Internal Errors

Unexpected internal conditions should provide sufficient diagnostic information to identify the affected subsystem.

Error handling shall avoid silently ignoring invalid states.

The exact exception/error-result strategy will be established during detailed design.

# 26. Determinism and Randomness

Where workloads use randomness, EMMUS shall support an explicit random seed.

For example:

Configuration

│

├── Workload

└── Random Seed

│

▼

Reproducible

Workload

This will allow simulations to be repeated with identical inputs.

A random workload without a specified seed may use a generated seed, but the resulting seed should be available in simulation results or logs when practical.

# 27. Performance Architecture

Performance measurement shall be built into the architecture without unnecessarily affecting simulation behavior.

The system should distinguish between: - Simulation time - Page-replacement decision time - Address-translation time - Workload-generation time - Reporting/output time

Where possible, measurement should be performed around defined interfaces rather than by embedding timing logic throughout algorithm implementations.

Performance measurements should allow meaningful comparisons between replacement algorithms.

# 28. Testing Architecture

The architecture shall support multiple levels of automated testing.

## 28.1 Unit Tests

Unit tests shall verify individual components such as: - Page-table behavior - Frame behavior - Physical-memory behavior - Page-replacement algorithms - Statistics - Workload generators - Configuration validation

## 28.2 Integration Tests

Integration tests shall verify interactions between components.

Examples include: - MMU + page table - MMU + physical memory - MMU + page replacement - Process + workload + MMU - Simulation + statistics

## 28.3 System Tests

System-level tests shall verify complete simulation scenarios.

Examples include: - Running a complete workload. - Comparing replacement algorithms. - Verifying expected page-fault counts. - Verifying dirty-page behavior. - Verifying configuration-driven execution.

## 28.4 Architecture Requirement

The core simulation engine shall be testable without the GUI.

This is a significant architectural requirement.

# 29. Proposed Component Relationship

The major relationships can be summarized as follows:

┌──────────────────┐

│ Configuration │

└────────┬─────────┘

│

▼

┌──────────────┐ ┌──────────────────┐

│ CLI / GUI │────────▶│ Simulation │

└──────────────┘ │ Coordinator │

└────────┬─────────┘

│

▼

┌──────────────────┐

│ Simulation │

│ Engine │

└────────┬─────────┘

│

┌─────────────┴─────────────┐

│ │

▼ ▼

┌──────────────┐ ┌──────────────┐

│ Process / │ │ MMU │

│ Workload │──────────▶│ │

└──────────────┘ └──────┬───────┘

│

┌──────────────────┼──────────────────┐

│ │ │

▼ ▼ ▼

┌────────────┐ ┌──────────────┐ ┌──────────────┐

│ Page Table │ │ Physical │ │ Page │

│ │ │ Memory │ │ Replacement │

└────────────┘ └──────────────┘ └──────────────┘

│ │ │

└──────────────────┼──────────────────┘

▼

┌──────────────┐

│ Statistics │

└──────────────┘

# 30. Proposed Software Component Organization

The exact source-tree organization will be established during implementation planning. However, the architecture should map naturally into independently managed components.

A conceptual organization is:

EMMUS/

│

├── apps/

│ ├── emmus-cli/

│ └── emmus-gui/

│

├── include/

│ └── emmus/

│ ├── application/

│ ├── simulation/

│ ├── memory/

│ ├── algorithms/

│ ├── statistics/

│ └── infrastructure/

│

├── src/

│ └── emmus/

│ ├── application/

│ ├── simulation/

│ ├── memory/

│ ├── algorithms/

│ ├── statistics/

│ └── infrastructure/

│

├── tests/

│ ├── unit/

│ ├── integration/

│ └── system/

│

├── config/

├── docs/

└── cmake/

This is a **conceptual organization**, not yet a final implementation requirement.

The final structure should be established after detailed design decisions have been made.

# 31. Architectural Extension Points

The architecture intentionally identifies several extension points.

## 31.1 Page-Replacement Algorithms

New policies should implement the common replacement abstraction.

Example future policies may include: - Random - MRU - LFU - Second Chance - Aging

Adding these should not require modification of the MMU’s fundamental operation.

## 31.2 Workloads

New workload generators should implement the workload abstraction.

## 31.3 Presentation

Additional interfaces should be able to use the simulation engine without modifying its core behavior.

## 31.4 Statistics

Additional metrics should be addable without rewriting the memory-management algorithms.

# 32. Architectural Decisions

The following architectural decisions are established by this document.

### AD-001 — Core Simulation Independence

The core simulation engine shall operate independently of the GUI.

### AD-002 — Interface-Based Page Replacement

Page-replacement algorithms shall be accessed through a common abstraction.

### AD-003 — Centralized Simulation Coordination

Simulation setup and execution shall be coordinated by an application-level component rather than being distributed across presentation components.

### AD-004 — Encapsulated Memory State

Page tables and physical-memory state shall be encapsulated within their respective subsystems.

### AD-005 — Configurable Simulation

Simulation parameters shall be provided through configuration rather than being hard-coded into the simulation engine.

### AD-006 — Reproducible Workloads

Randomized workloads shall support explicit random seeds.

### AD-007 — Automated Testing

Core subsystems shall be designed for independent automated testing.

### AD-008 — Presentation Independence

The simulation core shall not contain GUI-specific presentation logic.

# 33. Architectural Risks

The following risks shall be monitored during subsequent design and implementation.

### AR-001 — Overengineering

The project may become unnecessarily complex in an attempt to demonstrate architectural sophistication.

**Mitigation:** Prefer the simplest architecture that satisfies the requirements.

### AR-002 — GUI Coupling

GUI requirements could leak into core simulation components.

**Mitigation:** Maintain strict separation between presentation and core simulation.

### AR-003 — Excessive Abstraction

Interfaces may be introduced where they provide little practical value.

**Mitigation:** Introduce abstractions primarily at genuine variation or architectural boundaries.

### AR-004 — Performance Overhead

Instrumentation and abstraction could distort simulation performance.

**Mitigation:** Measure performance and separate measurement overhead from algorithm behavior where practical.

### AR-005 — Optimal Algorithm Coupling

The Optimal algorithm requires knowledge of future accesses that other algorithms do not require.

**Mitigation:** Design workload/replacement interaction carefully so that Optimal’s special information requirements do not contaminate the general replacement architecture.

### AR-006 — Scope Expansion

Additional features may cause the project to grow beyond a manageable portfolio project.

**Mitigation:** Evaluate future features against the original system objective and portfolio goals.

# 34. Architectural Constraints

The following constraints apply to subsequent design and implementation. 1. The core simulation engine shall not depend on the GUI. 2. Page-replacement algorithms shall be replaceable through a common abstraction. 3. Workloads shall be independently extensible. 4. Core memory-management components shall be independently testable. 5. Configuration shall not be hard-coded into core algorithms. 6. Simulation behavior should be reproducible when deterministic inputs are provided. 7. Memory state shall remain encapsulated within appropriate components. 8. Presentation logic shall remain separate from simulation logic. 9. Architectural abstractions shall have demonstrable value.

# 35. Traceability to Phase 1

The architecture directly supports the requirements established during Phase 1.

| **Phase 1 Requirement Area** | **Architectural Support**                  |
|------------------------------|--------------------------------------------|
| Virtual memory               | Virtual Memory subsystem                   |
| Physical memory              | Physical Memory subsystem                  |
| Page tables                  | Page Table subsystem                       |
| Address translation          | MMU                                        |
| Page faults                  | MMU / Page-Fault handling                  |
| Page loading                 | Memory Management subsystem                |
| Page eviction                | Physical Memory / Page Replacement         |
| FIFO                         | Page-Replacement subsystem                 |
| LRU                          | Page-Replacement subsystem                 |
| Clock                        | Page-Replacement subsystem                 |
| Optimal                      | Page-Replacement subsystem                 |
| Multiple processes           | Process Management                         |
| Workloads                    | Workload subsystem                         |
| Statistics                   | Statistics subsystem                       |
| Configuration                | Configuration subsystem                    |
| Logging                      | Infrastructure subsystem                   |
| Testability                  | Layered and modular architecture           |
| Extensibility                | Interface-based extension points           |
| Reproducibility              | Deterministic simulation and configuration |
| GUI                          | Presentation layer separated from core     |

Detailed requirement-to-component-to-test traceability will be maintained in the Requirements Traceability document.

# 36. Architecture Validation Criteria

The architecture shall be considered acceptable when the following questions can be answered positively: - Can the core simulator operate without the GUI? - Can a new page-replacement algorithm be added without modifying the MMU’s fundamental logic? - Can a new workload be added without modifying the simulation engine’s fundamental logic? - Can page-table behavior be unit tested independently? - Can physical-memory behavior be unit tested independently? - Can each replacement algorithm be tested independently? - Can a complete simulation be tested without user-interface automation? - Can simulation parameters be changed without recompiling core algorithms? - Can deterministic simulations be reproduced? - Can simulation statistics be collected without coupling the core to presentation logic? - Are component dependencies understandable? - Are architectural abstractions justified by actual variation or responsibility boundaries? - Does the architecture remain understandable to a developer reviewing the project for the first time?

**End of Document**
