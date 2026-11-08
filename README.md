# Ariadne Benchmarking

[![macOS Status](https://github.com/ariadne-cps/benchmarking/actions/workflows/macos.yml/badge.svg)](https://github.com/ariadne-cps/benchmarking/actions/workflows/macos.yml)
[![Ubuntu Status](https://github.com/ariadne-cps/benchmarking/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/ariadne-cps/benchmarking/actions/workflows/ubuntu.yml)
[![Windows Status](https://github.com/ariadne-cps/benchmarking/actions/workflows/win.yml/badge.svg)](https://github.com/ariadne-cps/benchmarking/actions/workflows/win.yml)
[![Sanitizers Status](https://github.com/ariadne-cps/benchmarking/actions/workflows/sanitizers.yml/badge.svg)](https://github.com/ariadne-cps/benchmarking/actions/workflows/sanitizers.yml)
[![Coverage Status](https://github.com/ariadne-cps/benchmarking/actions/workflows/coverage.yml/badge.svg)](https://github.com/ariadne-cps/benchmarking/actions/workflows/coverage.yml)
[![codecov](https://codecov.io/gh/ariadne-cps/benchmarking/branch/main/graph/badge.svg)](https://codecov.io/gh/ariadne-cps/benchmarking)

Ariadne Benchmarking provides composable C++20 helpers for measuring, validating,
summarizing and reporting performance experiments. **The code being measured
remains in its owning repository.** See the
[Ariadne dependency structure](https://github.com/ariadne-cps/ariadne/blob/main/doc/structure.md).

## Capabilities

- **Threading CLI:** inherit `ThreadingCommandLineInterface` and its
  `--concurrency N|max` support, Logging options and command-line parsing.
- **Time measurement:** steady-clock nanosecond/millisecond timing with explicit
  user-controlled setup, measurement and verification boundaries.
- **Samples and statistics:** elapsed samples, mean, median (including even
  counts), min, max, median absolute deviation and nearest-rank p95.
- **Calibration:** convert caller-supplied pilot timing to bounded repetitions,
  without implicitly running or mutating any experiment.
- **Comparison:** speedup (baseline/candidate) and geometric mean of ratios.
- **Reporting:** simple Logging-backed tabular summaries.
- **Optimization barrier:** keep measured results observable.

The library never changes a benchmark's measurement boundaries, warm-up policy,
sample ordering, or semantic checks. Those remain explicit in the caller.

## Dependencies

Benchmark support uses [Threading](https://github.com/ariadne-cps/threading),
which includes [Logging](https://github.com/ariadne-cps/logging) and
[Utility](https://github.com/ariadne-cps/utility) transitively.
[Build](https://github.com/ariadne-cps/build) supplies CMake infrastructure.
[Testing](https://github.com/ariadne-cps/testing) is required only by standalone
tests. No other Ariadne application module is required. Pinned gitlinks are
under `ss/bui`, `ss/thr`, and `ss/tst`.

**Benchmarking and Testing are independent opt-in development dependencies.**
Benchmarking is not linked to production libraries, not installed and not part
of the ordinary production build. Its repository never runs
`require_same_dependency_commit`; parents must also exclude the independent
Testing and Benchmarking gitlinks from cross-module commit-consistency checks.

## Directory conventions

- `include/benchmarking/`: reusable benchmark tools.
- `tests/`: isolated, semantic tests of the tools.
- `tutorial/`: a maintained, buildable C++ benchmark tutorial.
- `ss/`: pinned Build, Threading, Testing submodules.
- There is deliberately **no `benchmarks/` directory** in this repository.

## Build and test

```sh
git clone --recurse-submodules https://github.com/ariadne-cps/benchmarking.git
cd benchmarking
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DWARNINGS_AS_ERRORS=ON
cmake --build build-debug --parallel --target tests
ctest --test-dir build-debug --output-on-failure
```

## Tutorial

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target tutorial_benchmark
./build-release/tutorial/tutorial_benchmark --iterations 1000 --samples 7 --warmups 2
```

The tutorial illustrates `--concurrency` configuration, input preparation
outside the timed region, one
explicit measured operation, post-timing checksum verification, warm-up
exclusion, statistics and Logging-based reporting. Keep it updated whenever
the public API changes.

## Integration in consumer repositories

With a pinned Benchmarking gitlink and the corresponding Build refactoring,
call `setup_standalone_project(...)` once in the root CMake project.
Build's standalone orchestration initializes Benchmarking independently of
Testing. Benchmarking creates the aggregate `benchmarks` target and registers
the consumer's own `benchmarks/CMakeLists.txt` when present.
Link `ariadne-benchmarking` **only** to dedicated benchmark executables,
along with the owning project's linked library. Do not add the Benchmarking
interface to the production module's `INTERFACE_LINK_LIBRARIES`.

The initial migration is intentionally not backward compatible with the old
`setup_standalone_project_tests` API: repositories remain pinned to their old
Build commits until deliberately moved to the refactoring branch.

## Contributions

Please open issues or pull requests in this repository for common benchmarking
facilities. Domain-specific benchmark cases and oracles remain in their
respective Ariadne repositories.
