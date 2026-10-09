**EMMUS — DEVELOPER AND USER GUIDE**

**Project:** Enhanced Memory Management Unit Simulator (EMMUS)  
**Document Number:** 09  
**Document Title:** Developer and User Guide  
**Version:** 1.0  
**Status:** Initial Baseline  
**Date:** August 27, 2026  
**Project Type:** Software / Systems Engineering Portfolio Project

**Document Control**

**Revision History**

| **Version** | **Date**        | **Author**    | **Description**           |
|-------------|-----------------|---------------|---------------------------|
| 1.0         | August 27, 2026 | EMMUS Project | Initial document baseline |

**Document Status**

This document establishes the structure and content of the EMMUS Developer and User Guide.

Because EMMUS is being developed from the beginning, implementation-specific commands, configuration examples, screenshots, and operational procedures will be finalized as the software is implemented.

**1. Introduction**

**1.1 Purpose**

The purpose of this document is to provide instructions for building, configuring, executing, testing, and extending the Enhanced Memory Management Unit Simulator (EMMUS).

The document serves two related audiences:

1.  **Users** who want to build and operate EMMUS.

2.  **Developers** who want to understand, modify, test, and extend the system.

The guide is intended to provide sufficient information for a new developer to obtain the project, build the software, execute simulations, run tests, and understand the organization of the source code.

**1.2 Scope**

This document covers:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Project prerequisites</p></li>
<li><p>Source-code organization</p></li>
<li><p>Build procedures</p></li>
<li><p>Configuration</p></li>
<li><p>Application execution</p></li>
<li><p>Simulation execution</p></li>
<li><p>Workload configuration</p></li>
</ul></th>
<th><ul>
<li><p>Page-replacement algorithm selection</p></li>
<li><p>Test execution</p></li>
<li><p>Performance evaluation</p></li>
<li><p>Troubleshooting</p></li>
<li><p>Development workflow</p></li>
<li><p>Extension of EMMUS</p></li>
<li><p>Contribution practices</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

This document does not define system requirements or detailed software architecture. Those subjects are addressed by the other EMMUS engineering documents.

**2. Product Overview**

**2.1 System Description**

EMMUS is a software simulator that models memory-management operations within a computer system.

The simulator provides an environment for studying and comparing memory-management behavior without requiring modification of an actual operating system or hardware memory-management unit.

The system models concepts including:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Processes</p></li>
<li><p>Virtual addresses</p></li>
<li><p>Pages</p></li>
<li><p>Page tables</p></li>
<li><p>Physical frames</p></li>
<li><p>Physical memory</p></li>
</ul></th>
<th><ul>
<li><p>Memory accesses</p></li>
<li><p>Page faults</p></li>
<li><p>Page replacement</p></li>
<li><p>Dirty pages</p></li>
<li><p>Simulation workloads</p></li>
<li><p>Performance statistics</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**2.2 Supported Page-Replacement Algorithms**

The initial EMMUS implementation will support:

**FIFO**

First-In, First-Out replaces the page that has been resident for the longest period of time.

**LRU**

Least Recently Used replaces the page that has not been accessed for the longest period of time.

**Clock**

Clock uses reference information to provide a lower-overhead approximation of LRU behavior.

**Optimal**

Optimal uses knowledge of future memory references to select the page whose next access is furthest in the future.

Optimal is primarily intended as a theoretical reference algorithm because a real operating system cannot generally know future memory references.

**3. System Prerequisites**

**3.1 Hardware Requirements**

The minimum hardware requirements shall be established during implementation.

The expected development environment should provide:

- Modern x86-64 processor

- At least 8 GB of system memory

- Sufficient disk space for source code, build artifacts, test data, and documentation

The exact requirements may be revised as the project matures.

**3.2 Software Requirements**

The EMMUS development environment is expected to include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Linux or another supported development operating system</p></li>
<li><p>C++23-compatible compiler</p></li>
<li><p>CMake</p></li>
<li><p>Ninja or another supported build generator</p></li>
</ul></th>
<th><ul>
<li><p>Git</p></li>
<li><p>GoogleTest</p></li>
<li><p>Required third-party libraries</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

The exact versions used for the final project shall be documented in the project's build and development configuration.

**4. Project Structure**

The EMMUS repository shall be organized according to functional responsibility.

The expected high-level organization is:

EMMUS/

├── apps/

├── cmake/

├── data/

├── include/

├── src/

├── tests/

├── docs/

├── .github/

├── CMakeLists.txt

└── README.md

The exact directory structure may evolve during implementation.

**5. Source Code Organization**

**5.1 Application Layer**

Application-specific code shall be contained within the application layer.

Responsibilities may include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Application startup</p></li>
<li><p>Command-line processing</p></li>
</ul></th>
<th><ul>
<li><p>Application lifecycle</p></li>
<li><p>User-interface integration</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**5.2 Domain Layer**

The domain layer shall contain core concepts that represent the memory-management problem.

Examples include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Process</p></li>
<li><p>Page</p></li>
<li><p>Memory access</p></li>
</ul></th>
<th><ul>
<li><p>Address</p></li>
<li><p>Workload</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

Domain objects should remain as independent from user-interface concerns as practical.

**5.3 Memory Management Layer**

The memory-management subsystem shall contain components responsible for modeling memory operations.

Examples include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>MMU</p></li>
<li><p>Page table</p></li>
<li><p>Physical memory manager</p></li>
</ul></th>
<th><ul>
<li><p>Frame</p></li>
<li><p>Pager</p></li>
<li><p>Page-management components</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**5.4 Replacement Algorithm Layer**

Page-replacement algorithms shall be implemented behind a common abstraction.

The replacement subsystem shall contain:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Replacement-policy interface</p></li>
<li><p>FIFO implementation</p></li>
<li><p>LRU implementation</p></li>
<li><p>Clock implementation</p></li>
</ul></th>
<th><ul>
<li><p>Optimal implementation</p></li>
<li><p>Replacement-policy factory</p></li>
<li><p>Replacement statistics</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

This architecture allows replacement algorithms to be selected without requiring major changes to the rest of the memory-management system.

**5.5 Infrastructure Layer**

Infrastructure components shall provide shared services.

Examples include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Logging</p></li>
<li><p>Configuration management</p></li>
<li><p>File handling</p></li>
</ul></th>
<th><ul>
<li><p>Application services</p></li>
<li><p>Other reusable infrastructure functionality</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**5.6 Test Layer**

Tests shall be organized according to their verification level.

Expected categories include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Unit tests</p></li>
<li><p>Integration tests</p></li>
<li><p>System tests</p></li>
</ul></th>
<th><ul>
<li><p>Performance tests</p></li>
<li><p>Test fixtures</p></li>
<li><p>Test data</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**6. Building EMMUS**

**6.1 Configure the Project**

A typical out-of-source CMake configuration shall be used.

Example:

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

The exact configuration options shall be documented as they are established.

**6.2 Build the Project**

A typical build command is: cmake --build build

The build system shall compile the EMMUS libraries, application, and enabled test targets.

**6.3 Release Build**

A release configuration may be created using:

cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release

The release build shall be used for performance measurements unless a different configuration is explicitly documented.

**7. Running the Application**

The exact application executable name and command-line interface shall be documented after implementation.

The general execution model is: ./\<emmus-executable\>

Where applicable, command-line options shall allow the user to specify:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Configuration file</p></li>
<li><p>Workload</p></li>
<li><p>Replacement algorithm</p></li>
<li><p>Number of frames</p></li>
</ul></th>
<th><ul>
<li><p>Simulation parameters</p></li>
<li><p>Output location</p></li>
<li><p>Logging level</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

The final command-line interface shall be documented once implemented.

**8. Configuration**

**8.1 Configuration Purpose**

Configuration files shall provide a consistent method for specifying simulation parameters without requiring source-code changes.

Potential configuration parameters include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Page size</p></li>
<li><p>Physical-memory size</p></li>
<li><p>Number of frames</p></li>
<li><p>Number of processes</p></li>
<li><p>Replacement algorithm</p></li>
</ul></th>
<th><ul>
<li><p>Workload type</p></li>
<li><p>Workload size</p></li>
<li><p>Random seed</p></li>
<li><p>Statistics configuration</p></li>
<li><p>Output configuration</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**8.2 Replacement Algorithm Selection**

The selected page-replacement algorithm shall be configurable.

Example conceptual configuration:

replacement_algorithm: FIFO

Supported values shall correspond to the algorithms implemented by EMMUS.

**8.3 Configuration Validation**

EMMUS shall validate configuration values before beginning a simulation.

Invalid configurations shall be rejected with an appropriate diagnostic.

**9. Simulation Workflow**

A typical EMMUS simulation shall follow this sequence:

1.  Load configuration.

2.  Validate configuration.

3.  Initialize system components.

4.  Initialize physical memory.

5.  Initialize page tables.

6.  Initialize processes.

7.  Initialize the selected replacement algorithm.

8.  Generate or load the workload.

9.  Process memory accesses.

10. Perform address translation.

11. Handle page faults.

12. Perform page replacement when necessary.

13. Update page and frame state.

14. Record statistics.

15. Complete the workload.

16. Generate simulation results.

This sequence represents the conceptual simulation workflow. The final implementation may introduce additional processing steps.

**10. Memory Access Processing**

For each memory access, EMMUS shall determine:

1.  The originating process.

2.  The virtual address.

3.  The associated virtual page.

4.  The page-table entry.

5.  Whether the page is resident.

6.  The associated physical frame if resident.

7.  Whether a page fault has occurred.

8.  Whether a replacement is necessary.

9.  Whether the access is a read or write.

10. The resulting statistics.

**11. Page Fault Processing**

When a page fault occurs, the system shall determine whether a free physical frame is available.

If a free frame exists:

1.  Allocate the frame.

2.  Load the requested page.

3.  Update the page table.

4.  Notify the replacement policy.

5.  Continue the memory access.

If no free frame exists:

1.  Invoke the configured replacement algorithm.

2.  Select a victim.

3.  Determine whether the victim is dirty.

4.  Perform required eviction processing.

5.  Update page-table state.

6.  Load the requested page.

7.  Update replacement-policy state.

8.  Continue the memory access.

**12. Page-Replacement Operation**

The replacement subsystem shall provide a common interface for the supported algorithms.

The rest of EMMUS should interact with the replacement-policy abstraction rather than directly depending on a specific algorithm implementation.

This allows the replacement algorithm to be changed without requiring extensive modifications to the MMU or simulation engine.

**13. FIFO Operation**

FIFO maintains pages according to their load order.

When a replacement is required, the earliest eligible page is selected.

FIFO does not reorder pages based on subsequent accesses.

**14. LRU Operation**

LRU maintains information about recent page accesses.

When replacement is required, the least recently accessed eligible page is selected.

The implementation shall update its tracking state whenever a relevant page is loaded, accessed, or removed.

**15. Clock Operation**

Clock maintains reference information and a replacement pointer.

When replacement is required, the algorithm examines pages in a circular manner and provides referenced pages with a second chance according to the implementation design.

**16. Optimal Operation**

Optimal examines future memory references when selecting a victim.

The algorithm selects the page whose next use is furthest in the future, or a page that will not be used again, according to the defined implementation behavior.

The workload must therefore provide sufficient future-reference information.

**17. Workloads**

**17.1 Workload Types**

EMMUS shall support multiple workload patterns.

Initial workload types may include:

- Sequential

- Random

- Locality-based

- Repeated-reference

- Mixed

**17.2 Workload Reproducibility**

Random workloads should support a configurable seed.

When the same:

- Seed

- Workload configuration

- Number of accesses

- Process configuration

are used, the resulting workload should be reproducible.

This capability is important for debugging and performance evaluation.

**18. Simulation Statistics**

EMMUS shall collect statistics describing simulation behavior.

Potential statistics include:

- Total memory accesses

- Page faults

- Page-fault rate

- Page replacements

- Dirty evictions

- Total replacement execution time

- Average replacement execution time

- Total simulation execution time

Statistics shall be reset between independent simulations unless explicitly configured otherwise.

**19. Running Automated Tests**

The EMMUS test suite shall be built with the project.

A typical CTest command is:

ctest --test-dir build --output-on-failure

The exact command may change depending on the final build-directory structure.

**20. Unit Tests**

Unit tests verify individual components.

Examples include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><p>Frame tests</p>
<p>Page-table tests</p>
<p>Statistics tests</p>
<p>Configuration tests</p></th>
<th><p>FIFO tests</p>
<p>LRU tests</p>
<p>Clock tests</p>
<p>Optimal tests</p></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

Unit tests should be executed frequently during development.

**21. Integration Tests**

Integration tests verify interactions between components.

Examples include:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><p>MMU + Page Table</p>
<p>MMU + Physical Memory</p>
<p>MMU + Pager</p></th>
<th><p>Pager + Replacement Policy</p>
<p>Simulation + Workload</p>
<p>Simulation + Statistics</p></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

Integration tests should verify both successful operation and important failure conditions.

**22. System Tests**

System tests shall execute complete EMMUS simulations.

System tests should verify:

<table>
<colgroup>
<col style="width: 50%" />
<col style="width: 50%" />
</colgroup>
<thead>
<tr class="header">
<th><ul>
<li><p>Configuration</p></li>
<li><p>Initialization</p></li>
<li><p>Workload execution</p></li>
<li><p>Memory accesses</p></li>
<li><p>Page faults</p></li>
</ul></th>
<th><ul>
<li><p>Replacement</p></li>
<li><p>Statistics</p></li>
<li><p>Simulation completion</p></li>
<li><p>Result generation</p></li>
</ul></th>
</tr>
</thead>
<tbody>
</tbody>
</table>

**23. Performance Testing**

Performance testing shall be performed according to:

**Document 08 — Performance Evaluation Report**

Performance experiments should use controlled and repeatable configurations.

The same workload should be used when directly comparing replacement algorithms.

**24. Developer Workflow**

The recommended development workflow is:

1.  Review the applicable requirements.

2.  Review the relevant architecture and design.

3.  Implement the required functionality.

4.  Write or update unit tests.

5.  Build the project.

6.  Run automated tests.

7.  Correct defects.

8.  Run regression tests.

9.  Update requirements traceability.

10. Update documentation where necessary.

11. Commit the completed work.

Development should follow the standards established in **Implementation and Development Standards**.

**25. Adding a New Replacement Algorithm**

EMMUS is intended to allow additional page-replacement algorithms to be added without major architectural changes.

A developer adding an algorithm should generally:

**Step 1**

Implement the common replacement-policy interface.

**Step 2**

Create the algorithm implementation.

**Step 3**

Implement required lifecycle operations, such as:

- Page loaded

- Page accessed

- Page removed

- Victim selection

**Step 4**

Add unit tests.

**Step 5**

Add integration tests.

**Step 6**

Add the algorithm to the policy factory or registration mechanism.

**Step 7**

Add configuration support.

**Step 8**

Add performance experiments.

**Step 9**

Update requirements traceability.

**Step 10**

Update documentation.

**26. Adding a New Workload Type**

A new workload generator should:

1.  Follow the workload abstraction defined by the project.

2.  Generate valid memory references.

3.  Support reproducibility where appropriate.

4.  Include unit tests.

5.  Include representative integration testing.

6.  Document configuration parameters.

7.  Be included in performance evaluation where appropriate.

**27. Testing Best Practices**

Developers should:

- Test new behavior as it is implemented.

- Avoid relying exclusively on system tests.

- Prefer deterministic unit tests.

- Test both valid and invalid inputs.

- Include boundary conditions.

- Preserve regression tests for significant defects.

- Keep tests independent.

- Avoid unnecessary test coupling.

- Verify expected results rather than implementation details when possible.

**28. Debugging**

When investigating a defect, developers should first determine which subsystem is responsible.

Recommended debugging sequence:

1.  Reproduce the issue.

2.  Identify the failing operation.

3.  Determine the affected subsystem.

4.  Review relevant unit tests.

5.  Add a focused test if necessary.

6.  Inspect logging and state transitions.

7.  Correct the underlying defect.

8.  Run the focused test.

9.  Run the regression suite.

10. Document the defect and resolution when appropriate.

**29. Logging**

Logging shall be used to provide diagnostic information without unnecessarily coupling application behavior to logging implementation details.

Useful logging information may include:

- Simulation initialization

- Process creation

- Page faults

- Frame allocation

- Victim selection

- Page eviction

- Dirty-page detection

- Simulation completion

- Configuration errors

Logging verbosity should be configurable where practical.

**30. Error Handling**

EMMUS shall use controlled error handling.

Errors should provide enough information to identify:

- What operation failed

- Which component detected the failure

- Relevant identifiers or configuration values

- Whether execution can safely continue

The system should avoid silently ignoring significant errors.

**31. Troubleshooting**

**31.1 CMake Configuration Failure**

If CMake configuration fails:

1.  Review the CMake error message.

2.  Verify required dependencies.

3.  Confirm the compiler is available.

4.  Confirm the requested C++ standard is supported.

5.  Delete the build directory if necessary.

6.  Reconfigure the project.

**31.2 Compilation Failure**

If compilation fails:

1.  Identify the first compiler error.

2.  Verify include paths.

3.  Verify namespace names.

4.  Verify header/source consistency.

5.  Check recent changes.

6.  Rebuild after correcting the underlying issue.

Later compiler errors may be consequences of the first failure and should not necessarily be addressed independently.

**31.3 Test Failure**

If a test fails:

1.  Run the individual test.

2.  Review the expected and actual results.

3.  Determine whether the defect is in the test or implementation.

4.  Reproduce the issue.

5.  Correct the underlying problem.

6.  Run the affected test again.

7.  Run the complete regression suite.

**31.4 Unexpected Page-Replacement Result**

When a replacement algorithm produces an unexpected victim:

1.  Verify the workload.

2.  Verify the initial frame state.

3.  Verify page-load order.

4.  Verify page-access events.

5.  Inspect algorithm state.

6.  Verify the expected result manually.

7.  Execute the algorithm's unit tests.

8.  Execute the corresponding integration test.

**32. Repository and Version Control**

Git shall be used for source-code version control.

Commits should:

- Represent coherent changes.

- Use descriptive messages.

- Avoid unrelated modifications.

- Preserve a buildable project where practical.

- Include relevant tests with functional changes.

Large architectural changes should be clearly documented.

**33. Continuous Integration**

Where configured, CI should automatically:

1.  Check out the repository.

2.  Configure the build.

3.  Compile the project.

4.  Compile the tests.

5.  Execute automated tests.

6.  Report failures.

CI provides an additional mechanism for detecting regressions.

**34. Documentation Requirements**

When functionality changes, associated documentation shall be reviewed.

Documentation updates may be required for:

- Requirements

- Architecture

- Detailed design

- Configuration

- Testing

- Performance evaluation

- User procedures

- Developer procedures

- README content

Documentation should describe the implemented system accurately rather than describing intended behavior that has not been implemented.

**35. Security and Reliability Considerations**

Although EMMUS is primarily an educational and portfolio simulation system, software reliability remains important.

The implementation should:

- Validate external inputs.

- Avoid undefined behavior.

- Avoid uncontrolled resource consumption.

- Handle invalid configuration safely.

- Prevent invalid memory-management states where practical.

- Provide meaningful diagnostics.

**36. Reproducibility**

A developer or evaluator should be able to reproduce an EMMUS experiment using documented information.

A reproducible experiment should identify:

- EMMUS version or Git commit

- Build configuration

- Configuration file

- Algorithm

- Workload

- Workload seed

- Number of frames

- Page size

- Number of processes

- Operating environment

This is especially important for performance evaluation.

**37. Portfolio Demonstration**

EMMUS is intended to demonstrate engineering capability as well as software functionality.

A portfolio demonstration should emphasize:

- Requirements engineering

- System architecture

- Object-oriented design

- C++23 development

- Memory-management concepts

- Page-replacement algorithms

- Automated testing

- Requirements traceability

- Performance evaluation

- Build automation

- Documentation

- Version control

The project should demonstrate the complete engineering process from requirements through implementation and verification.

**38. Recommended Demonstration Scenario**

A representative demonstration should:

1.  Start EMMUS.

2.  Load a documented workload.

3.  Select a replacement algorithm.

4.  Configure physical-memory capacity.

5.  Execute the simulation.

6.  Display memory-management statistics.

7.  Repeat using another replacement algorithm.

8.  Compare the results.

9.  Explain the observed differences.

A small deterministic workload should be used for demonstrations because its expected behavior can be independently verified.

**39. Maintenance**

Future maintenance activities may include:

- Defect correction

- Algorithm improvements

- New replacement algorithms

- New workload generators

- Additional statistics

- User-interface improvements

- Performance improvements

- Test expansion

- Documentation updates

Changes should preserve the architectural boundaries established by the project.

**40. Future Extension Opportunities**

Potential future enhancements include:

- Additional page-replacement algorithms

- Working-set algorithms

- Aging algorithms

- Configurable memory hierarchies

- TLB simulation

- Multi-level page tables

- Memory-access visualization

- Interactive simulation stepping

- Expanded workload models

- Exportable performance results

- Automated performance benchmarking

- Additional process-scheduling models

These features are potential extensions and are not automatically part of the initial EMMUS scope.

**41. Relationship to Other EMMUS Documents**

This document provides operational and development guidance for the EMMUS project.

| **Document**                             | **Relationship**                                   |
|------------------------------------------|----------------------------------------------------|
| System Requirements Specification        | Defines system requirements                        |
| System Architecture Document             | Defines system structure                           |
| Detailed Software Design Document        | Defines detailed implementation design             |
| Implementation and Development Standards | Defines development practices                      |
| Implementation and Verification Plan     | Defines implementation and verification activities |
| Requirements Traceability Matrix         | Provides requirements traceability                 |
| Verification and Validation Report       | Documents verification and validation              |
| Performance Evaluation Report            | Defines and records performance evaluation         |
| Project README                           | Provides the public-facing project overview        |

**42. Document Maintenance**

This document shall be updated as the implementation evolves.

Implementation-specific information shall replace the initial placeholders once the associated functionality exists.

The guide shall be reviewed whenever changes affect:

- Build procedures

- Configuration

- Application operation

- Testing

- Repository structure

- Developer workflow

- Supported algorithms

- Workload configuration

**43. Conclusion**

The EMMUS Developer and User Guide provides the operational and development framework for using and extending the Enhanced Memory Management Unit Simulator.

The guide establishes procedures for:

- Building EMMUS

- Configuring simulations

- Running workloads

- Selecting replacement algorithms

- Executing automated tests

- Performing performance experiments

- Debugging the system

- Extending the simulator

- Maintaining the project

As EMMUS progresses from architectural design through implementation and verification, this document will transition from an initial baseline into the definitive operational guide for the completed system.

The final version should allow a new user to operate EMMUS and a new developer to understand and extend the software without requiring undocumented project knowledge.

**End of Document**
