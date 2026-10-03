# EMMUS

**Enhanced Memory Management Unit Simulator (EMMUS)** is a C++23 project that models page-based virtual-memory behavior. Its core library brings together simulated processes and workloads, virtual pages, page tables, physical frames, an MMU, four replacement-policy implementations, and simulation statistics.

The project is intended as an educational and engineering portfolio project—not an operating-system memory manager. Its most useful review paths are:

1. **What is modeled?** See [System architecture](docs/architecture/system-architecture.md).
2. **How do the requirements map to code and tests?** See the [Requirements traceability matrix](docs/requirements/requirements-traceability-matrix.md).
3. **How was behavior verified and performance measured?** See [Testing and performance](docs/verification/testing-and-performance.md).
4. **How can I reproduce a build and test run?** See [Build and development workflow](docs/development/build-and-workflow.md).

## At a glance

| Area | Current implementation |
|---|---|
| Language/build | C++23, CMake 3.24+, checked-in Ninja `debug` and `release` presets |
| Core | Installable static library target `EMMUS::Core` |
| Memory model | Per-process virtual-page registrations, a page table, shared physical-frame manager, MMU access/fault handling |
| Replacement policies | FIFO, LRU, Clock, and Optimal policy classes |
| Workloads | Sequential, seeded random, locality, mixed, and multi-process composition |
| Verification | GoogleTest suites registered with CTest: unit, integration, and system |
| Presentation | Optional text-rendering view library for memory, activity, and statistics; **not** a windowed GUI application |
| CLI | Optional target is scaffolded, but currently cannot be enabled because `apps/emmus-cli/main.cpp` is absent |

## Quick start

Requirements: CMake 3.24 or newer, Ninja, a C++23-capable compiler, Git, and network access for the first configure (GoogleTest is fetched at a pinned revision).

```sh
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug --no-tests=error
```

For complete setup, optional targets, installation, test filters, and CI behavior, follow [Build and development workflow](docs/development/build-and-workflow.md).

## Important scope notes

- This is a deterministic simulation of page/frame mappings and counters, not a byte-accurate memory emulator or a real MMU.
- The standard simulation runner does not pass a future-reference trace to the Optimal policy. Its results therefore must not be presented as a true optimal lower bound. Details and other limitations are in [System architecture](docs/architecture/system-architecture.md).
- Timing support collects measurements, but the repository does not contain a published benchmark-results dataset or a dedicated benchmark executable. See [Testing and performance](docs/verification/testing-and-performance.md).
- The checked-in system tests do exercise complete headless simulation runs; GUI view tests are separately enabled with the optional GUI library.

## Project layout

```text
include/emmus/       Public core interfaces
src/emmus/           Core implementation
apps/emmus-gui/      Optional text-rendering view library
apps/emmus-cli/      CLI target scaffold (entry point not implemented)
tests/unit/          Component-level tests
tests/integration/   Cross-component tests
tests/system/        Complete headless simulation scenarios
tests/gui/           Optional presentation-view tests
cmake/               Warning, sanitizer, and install/package modules
docs/                Architecture, requirements, workflow, and evidence
```

The project uses an incremental engineering approach: requirements and traceability are maintained in `docs/requirements/`, tests are discovered by CMake/GoogleTest and run through CTest, and CI builds/tests both Debug and Release presets before staging an install. The detailed documentation linked above distinguishes implemented behavior from known gaps and future work.
