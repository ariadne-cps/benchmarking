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
  counts), min, max, median absolute deviation, sample variance, sample
  standard deviation and nearest-rank p95.
- **Calibration:** convert caller-supplied pilot timing to bounded repetitions,
  without implicitly running or mutating any experiment.
- **Comparison:** speedup (baseline/candidate) and geometric mean of ratios.
- **Parameter matrices:** all custom benchmark arguments accept ordered typed
  lists; a lazy Cartesian product preserves duplicates and exposes each case.
- **Reporting:** Logging-backed summaries with complete parameter sequences
  in the header and one labelled row per configuration. The standard
  `--summary` option chooses report columns and order; the default is
  `min,max,mean,mad`. Also available: `median,p95,stddev,variance,count`.
- **Dynamic-width tables:** the generic table engine sizes every column by
  the longest formatted cell or heading, supports grouping, separator rules,
  optional minimum widths, and arbitrary descriptive and numeric columns.
- **Warm-ups:** the standard `--warmups N` option defaults to zero.
  `Runner` performs each warm-up with preparation, timing and verification,
  excludes its timing from statistics and emits progress through Logging.
- **Progress:** Logging details for configurations, executions and samples.
- **Optimization barrier:** keep measured results observable.

The benchmark author supplies separate preparation, operation and verification
callbacks; `Runner` applies the global warm-up count before measured samples.
Preparation and verification remain outside the timed region, and warm-ups
are checked but never included in reported statistics.

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

### Shared Build compatibility

Benchmarking is now pinned to the shared Build mainline. Standalone
C++ tests use `setup_standalone_project()` as provided by the merged Build mainline; the earlier test-only setup macro was superseded.
Benchmarking continues to own its root-scope
`ariadne_register_project_benchmarks` registration helper and
remains an independent opt-in benchmarking module. Its pinned
Build, Threading and Testing submodules are aligned with the
shared Build revision used across Ariadne, without making
Benchmarking a runtime dependency of production libraries.

## Directory conventions

- `include/benchmarking/`: reusable benchmark tools.
- `tests/`: isolated, semantic tests of the tools.
- `tutorial/`: two progressive, buildable C++ tutorials (standalone, then A/B).
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
cmake --build build-release --target tutorial_benchmarking tutorial_comparison
./build-release/tutorial/tutorial_benchmarking --iterations 1000 --samples 7 --warmups 2
./build-release/tutorial/tutorial_comparison --verbosity 1
```

The tutorials progress from a feature-rich single benchmark (including
parameter matrices, custom summary and table layout) to a deliberately minimal
A/B comparison with two implementation templates and one paired report.
See [tutorial/README.md](tutorial/README.md) for the walkthrough.

## Integration in consumer repositories

With a pinned Benchmarking gitlink and the corresponding Build refactoring,
call `setup_standalone_project(...)` once in the root CMake project.
Build's standalone orchestration initializes Benchmarking independently of
Testing. Benchmarking creates the aggregate `benchmarks` target and registers
the consumer's own `benchmarks/CMakeLists.txt` when present.
Link `ariadne-benchmarking` **only** to dedicated benchmark executables,
along with the owning project's linked library. Do not add the Benchmarking
interface to the production module's `INTERFACE_LINK_LIBRARIES`.

The standalone test setup uses the shared
`setup_standalone_project` API; benchmark registration remains
opt-in and isolated from production targets.

## Contributions

Please open issues or pull requests in this repository for common benchmarking
facilities. Domain-specific benchmark cases and oracles remain in their
respective Ariadne repositories.


## Tabular output for comparisons

The generic table API supports FloatDP/FloatMP-style reports without
incorporating backend-specific ratio calculations:

~~~cpp
using namespace Ariadne::Benchmarking;
TableOptions options = TableOptions::ruled();
options.group_columns = {0u};
Table table({
    {"Backend"}, {"Operation"},
    {"Bits", 0u, TableAlignment::Right},
    {"Median (ns)", 0u, TableAlignment::Right}
}, options);
table.add_row({"floatdp", "add_nearest", "53", Table::number(12.345)});
table.add_row({"floatmp", "add_nearest", "53", Table::number(23.456)});
std::string text = table.str();
~~~

Each column width is the maximum length of its heading and every already
formatted displayed cell; no default fixed width is imposed. Clients can
choose left/right alignment, column separators, optional top/header/group/
bottom rules, and grouping by any number of descriptive columns. Explicit
minimum column widths are available when a fixed layout is desired.

The Report class uses the same engine. Its old two-descriptor add API remains
available, while set_dimension_columns, set_table_options, and add_row allow
multidimensional tables. Computations such as FloatMP/FloatDP slowdown,
geometric mean, weighted total-time slowdown, and worst case remain in the
owning benchmark, which can format them through the generic Table.

## Comparing implementations

`ComparisonBattery<ImplA,ImplB,...>` instantiates the same templated workload
for each concrete type and uses `Runner` to measure each registered operation.
Each configuration may provide its own inputs. `ComparisonResults` pairs
implementations using operation and configuration, rejects missing or duplicate
rows, and reports candidate/reference slowdown or its reciprocal speedup.

`ComparisonReports` supports any number of independently configured views:
raw timing statistics, paired comparisons and aggregate comparisons. Each view
chooses its own columns and order. Aggregation offers geometric mean slowdown,
total-time ratio and worst slowdown, either per configuration or over all
configurations. The tutorial intentionally demonstrates only the simplest A/B
case; full N-way and configurable-report behavior is exercised by tests.
