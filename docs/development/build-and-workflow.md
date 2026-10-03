# Build and development workflow

## Toolchain and dependencies

| Requirement | Project configuration |
|---|---|
| CMake | 3.24 or newer (`cmake_minimum_required` and preset metadata) |
| Build system | Ninja is selected by the checked-in CMake presets |
| C++ compiler | A compiler supporting the project's required C++23 features; CMake requests C++23 with extensions disabled |
| Git/network | Needed for a fresh checkout and first test-enabled configure |
| GoogleTest | Fetched by CMake `FetchContent` at pinned revision `52eb8108c5bdec04579160ae17225d66034bd723`; a system GoogleTest install is not required |

The repository does not declare an exact minimum compiler version. Use a current compiler with C++23 support for the features used by the source. The standard project presets enable tests by default, so the first configure downloads the pinned GoogleTest source into the build directory.

## Get the source

```sh
git clone https://github.com/SeanMorgan72/EMMUS.git
cd EMMUS
```

Run all project commands from the repository root.

## Configure, build, and test

The presets provide independent Ninja trees at `build/debug` and `build/release`.

```sh
# Debug
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug --no-tests=error

# Release
cmake --preset release
cmake --build --preset release --parallel
ctest --preset release --no-tests=error
```

The `--no-tests=error` guard makes an empty CTest discovery fail rather than look like a passing test run. CMake's `gtest_discover_tests()` registers individual GoogleTest cases after each test executable is built.

## CMake options

Options are controlled with `-DNAME=value` at configure time. The root project defaults are:

| Option | Default | Meaning |
|---|---:|---|
| `EMMUS_BUILD_TESTS` | `ON` | Include EMMUS GoogleTest targets (also requires `BUILD_TESTING`). |
| `BUILD_TESTING` | CTest default, `ON` | Master CTest switch. |
| `EMMUS_BUILD_CLI` | `OFF` | Request the CLI target. **Not currently buildable:** enabling it fails configuration because `apps/emmus-cli/main.cpp` does not exist. |
| `EMMUS_BUILD_GUI` | `OFF` | Build `EMMUS::GUI`, an optional static text-view library, plus its view test. It does not create a windowed GUI. |
| `EMMUS_ENABLE_WARNINGS` | `ON` | Enable the project warning set for supported compilers. |
| `EMMUS_ENABLE_SANITIZERS` | `OFF` | Enable AddressSanitizer and UndefinedBehaviorSanitizer on GNU/Clang-family targets; the CMake module warns and returns for MSVC. |
| `EMMUS_ENABLE_INSTALL` | `ON` | Configure install/export rules for the core library and headers. |

To build and test the optional view library without changing a checked-in preset:

```sh
cmake -S . -B build/gui -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DEMMUS_BUILD_GUI=ON \
  -DEMMUS_BUILD_TESTS=ON
cmake --build build/gui --parallel
ctest --test-dir build/gui --output-on-failure --no-tests=error
```

The GUI test is registered with the `gui` label. It tests the statistics view; it does not test or imply a graphical shell.

## Running focused tests

```sh
ctest --test-dir build/debug -L unit --output-on-failure
ctest --test-dir build/debug -L integration --output-on-failure
ctest --test-dir build/debug -L system --output-on-failure
```

Build first, because discovered GoogleTest cases depend on built executables. Include `--no-tests=error` for release/CI verification. When the GUI option was enabled, add `-L gui` to run its view test. `ctest --test-dir build/debug -N` lists discovered tests and labels.

## Installation

The CI pipeline stages a Debug and Release install into separate temporary prefixes. To install a local build into a project-local staging directory:

```sh
cmake --install build/release --prefix "$PWD/build/install"
```

The install rules export the core library, public headers, CMake target namespace `EMMUS::`, and package config/version files under the platform's standard install directories. Installation is for consuming the core library; optional application installation is conditional on those targets existing.

## Continuous integration

`.github/workflows/ci.yml` runs on pushes and pull requests. Its Ubuntu matrix configures the `debug` and `release` presets, builds in parallel, executes the full discovered suite with `ctest --preset <preset> --no-tests=error`, and installs each build to a temporary prefix. This verifies both build configurations and install rules, but does not constitute a performance benchmark.

## Development and review workflow

1. Identify or update the user story/requirement and define observable acceptance behavior.
2. Make the narrowest cohesive core change; keep policy, MMU, physical-memory, workload, statistics, and presentation responsibilities separate.
3. Add deterministic tests at the right boundary: unit for a component, integration for subsystem interactions, and system for a configured full simulation.
4. For randomized cases, specify a seed. Prefer verifying public outcomes over private implementation details.
5. Update the [traceability matrix](../requirements/requirements-traceability-matrix.md) and related design/evidence documentation with source and test references.
6. Build and run the focused test label, then run the full Debug/Release CI-equivalent commands before review.
7. Record any known gap as a gap rather than implying that an untested feature is complete.

The existing [test guide](../../tests/README.md) gives test naming, arrange/act/assert, fixture, determinism, and assertion conventions. Production code must not depend on test code.

## Repository map

```text
.github/workflows/       CI build/test/install pipeline
apps/                    Optional application targets and text views
cmake/                   CMake warning, sanitizer, package/install helpers
docs/                    Maintained project documentation
include/emmus/           Public core headers
src/emmus/               Core implementation
tests/unit/              Component behavior
tests/integration/       Cross-component behavior
tests/system/            Complete simulation scenarios
tests/gui/               Optional presentation-view behavior
```
