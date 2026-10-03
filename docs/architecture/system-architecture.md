# System architecture and design

## Purpose and scope

EMMUS is an educational simulator for exploring page-based virtual-memory management and comparing replacement-policy behavior over controlled access workloads. It makes the path from a process memory reference to a page-table lookup, frame assignment, page fault, and possible replacement observable and testable.

EMMUS does **not** emulate an operating-system kernel, CPU, TLB, disk, or byte contents. A simulated access updates page/frame state and statistics; it does not read or write application data. This distinction is important when interpreting both its output and timing.

## Requirements and scope

The implementation supports these user-visible capabilities:

- Configure page size, frame count, process count, pages per process, policy, workload, access count, and random seed.
- Register virtual pages for simulated processes and submit virtual-address accesses.
- Allocate free frames deterministically, handle page faults, and replace a resident page when no free frame remains.
- Track page residency, dirty and referenced flags, memory accesses, faults, replacements, dirty evictions, and elapsed time.
- Generate sequential, random, locality, and mixed workloads; combine per-process workloads in a multi-process run.
- Compare page-fault and replacement-time statistics across policy runs.
- Provide optional text renderers for memory utilization, activity events, and simulation statistics.

It does not provide process scheduling, protection levels, demand-zero contents, swap/write-back I/O, a command-line interface, a graphical window, or a hardware-accurate physical-address/data path.

## Runtime structure

```text
SimulationConfiguration / BenchmarkConfiguration
                    |
                    v
Simulation ---- ProcessManager + Workload
    |                 |
    |                 v
    |           MemoryAccessExecutor
    |                 |
    v                 v
MemoryManagementUnit <---- ordered MemoryAccess records
    |         |       |
    |         |       +---- IPageReplacementPolicy
    |         |               FIFO / LRU / Clock / Optimal
    |         |
    |         +---- PageTable <----> PageTablePhysicalMemoryIntegration
    |                                      |
    +--------------------------------------+
                                           v
                                  PhysicalMemoryManager
                                    frames and ownership

SimulationResult / statistics / activity log
                    |
                    v
       optional text-rendering view library
```

`EMMUS::Core` owns the simulation and has no dependency on the optional view library. The simulation accepts a progress callback and records an activity log, which makes the core usable in headless tests and lets presentation code consume snapshots without taking ownership of memory-management state.

## Component responsibilities

| Component | Responsibility and boundary |
|---|---|
| `Simulation` | Validates configuration, creates processes and registered pages, selects policy/workload, executes the generated access stream, and assembles a result. Each `run()` resets and recreates the mutable simulation state for a fresh run. |
| Workload interfaces and implementations | Produce `MemoryAccess` values for Sequential, Random, Locality, and Mixed workloads. A `MultiProcessWorkload` combines per-process streams. Seeded workloads make generated sequences reproducible. |
| `MemoryAccessExecutor` | Sends each access to the MMU, stores per-access results, aggregates execution counters, and invokes an optional progress callback. |
| `MemoryManagementUnit` | Resolves a process's virtual page, rejects unregistered or foreign pages, checks mapping consistency, processes resident accesses and faults, coordinates free-frame allocation/replacement, and updates page state/statistics. |
| `PageTable` | Stores virtual `PageId` to physical `FrameId` mappings only. It does not allocate frames, select victims, or perform access handling. |
| `PageTablePhysicalMemoryIntegration` | Keeps frame allocation/release and page-table mapping operations coordinated and provides consistency checks across those structures. |
| `PhysicalMemoryManager` / `Frame` | Own a fixed collection of numbered frames. Allocation selects the lowest-numbered free frame; the manager returns no frame rather than making a replacement decision when full. |
| `Page` | Represents one page's identity, process owner, optional resident frame, dirty flag, and reference flag. Page state changes are separate from frame ownership and policy choice. |
| Replacement policy interface | Receives load, access, and removal notifications; selects a victim frame; and reports policy-level statistics. |
| Statistics and activity log | Aggregate access/fault/replacement/timing values and retain typed simulation events for later presentation. |
| `emmus-gui` | Optional static library containing text-based view/model classes. It formats snapshots; it is not a GUI framework or runnable application. |

## Memory-access and page-fault flow

```text
MemoryAccess(process, virtual address, operation)
        |
        v
Split address into virtual-page number + offset
        |
        v
Resolve page in that process's registered virtual-page map
        |
   invalid/foreign --------------------> failed result; no page load
        |
        v
Check PageTable <-> PhysicalMemory consistency
        |
   resident ---------------------------> update access/page state
        |
   not resident (page fault)
        |
        +-- free frame exists ----------> allocate lowest free frame
        |
        +-- memory full ---------------> ask policy for victim frame
                                             |
                                             v
                                    release/unmap victim
                                    map requested page
                                    record dirty eviction if needed
        |
        v
Return access result and update counters/log
```

The access handler decomposes a virtual address using the configured page size and resolves the virtual page within the owning process. The current implementation does not use the page offset to model bytes or calculate a data-bearing physical address; page/frame association is the modeled translation result. An unmapped page fault is counted before allocation/replacement is attempted, while a mapping/consistency failure can still make the access unsuccessful.

The MMU prefers a free frame and must not call the replacement policy in that case. When memory is full, it asks the selected policy for a frame, checks the victim and mapping state, unmaps/releases the old page, maps the requested page, and notifies the policy. It records dirty eviction when the victim's dirty flag is set. The dirty state is bookkeeping only; there is no simulated disk write or persistence layer.

## Replacement behavior

| Policy | Implemented decision rule | State maintained |
|---|---|---|
| FIFO | Select the oldest resident load entry; accesses do not reorder it. | Residency queue. |
| LRU | Select the least recently accessed entry. A load/access moves a page to most-recent position. | Ordered list plus page/frame iterator lookup maps. |
| Clock | Inspect a circular collection. A set reference bit is cleared and receives a second chance; a clear bit is eligible. Advance the hand after inspection/selection. | Per-resident reference bit and clock-hand index. |
| Optimal | Among resident pages, select the one whose next reference is farthest away; never-used-again pages are preferred. Ties retain earlier residency order. | Resident entries plus an explicitly provided future reference sequence. |

### Optimal-policy limitation

`OptimalPageReplacementPolicy` exposes `setReferenceSequence()` and its unit/integration tests supply traces directly. The production `Simulation` path creates the policy but does **not** call that method or provide a future access sequence. Consequently, a normal `SimulationComparison` run includes an Optimal policy slot with no future-use information; its behavior is not a valid theoretical optimal baseline. Do not interpret such results as proof of Optimal's minimum-fault property. Passing the complete generated reference trace into the simulation policy is a future improvement.

## Workloads and reproducibility

`SimulationConfiguration` carries workload settings, including random seed and locality parameters. Sequential and Random runs generate a configured number of accesses; Locality has temporal/spatial strengths and a working-set size; Mixed combines configured segments without nested Mixed segments. For multiple processes, the configured total access count is divided across processes, with any remainder assigned to the earliest process. Per-process random streams use the configured seed plus the process index; Mixed segments derive their own deterministic seeds.

`BenchmarkConfiguration::withReplacementPolicy()` copies the benchmark parameters and changes only the selected policy. `SimulationComparison` repeats the benchmark across FIFO, LRU, Clock, and Optimal. The seeded random workload and configuration are intended to remain equivalent across those runs, subject to the Optimal caveat above.

## Design decisions

1. **Policy interface and factory** — isolates policy-specific state and selection from the MMU and makes policy implementations independently testable.
2. **Explicit page/frame ownership boundaries** — the page table maps identifiers; the physical manager owns frames; the MMU coordinates access state; the policy chooses victims. This avoids a single manager owning unrelated decisions.
3. **Strongly typed identifiers and configuration value objects** — distinguish process, page, frame, address, and access concepts and validate configuration before execution.
4. **Deterministic choices where practical** — frame allocation uses the lowest free frame, policy ties have deterministic rules, and generated randomized workloads accept explicit seeds.
5. **Headless core and callback-based progress** — keeps simulation tests and engine behavior independent of presentation code.
6. **CMake target isolation** — core is always the main target; CLI and views are optional and tests are split into CTest labels.

## Presentation layer

The optional `EMMUS::GUI` target currently contains text formatters:

- `PhysicalMemoryVisualizationView`: frame allocation, page/frame identifiers, utilization, and compact occupancy.
- `SimulationActivityVisualizationView`: typed activity events such as access, fault, replacement, frame assignment, dirty eviction, and status.
- `SimulationStatisticsVisualizationView`: progress/completion snapshots, accesses, fault rate, replacements, dirty evictions, utilization, elapsed time, replacement time, and an optional baseline comparison.

The view classes render strings. The repository has no `main()` for a GUI, Qt dependency, window/event loop, interactive controls, or rendered graphical widgets. GUI-view tests are available when `EMMUS_BUILD_GUI=ON`.

## Known limitations and future work

- Supply the generated future trace to Optimal and verify fault-count bounds against that trace.
- Model actual physical-address formation and, if desired, byte-level data and write-back behavior.
- Add an executable CLI and a real interactive GUI only if those products are in scope.
- Add a dedicated, repeatable benchmark runner and checked-in or published results with compiler, build type, workload, repetitions, and measurement methodology.
- Evaluate timing variability and instrumentation cost before using nanosecond-level results for policy ranking.
- Add validation for documentation links/traceability and expand end-to-end checks around failure/rollback paths.

## Source map

Public interfaces are under `include/emmus/`; implementations are under `src/emmus/`. Application targets are under `apps/`; tests and their conventions are described in [Testing and performance](../verification/testing-and-performance.md). See the [requirements matrix](../requirements/requirements-traceability-matrix.md) for implementation-to-test references.
