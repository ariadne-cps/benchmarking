# C++ tutorials

The tutorials are intentionally progressive: first an ordinary benchmark,
then a minimal A/B comparison. Both are explicit CMake targets excluded from
the ordinary production build and the aggregate `benchmarks` target.

## 1. Standalone benchmark: `tutorial_benchmarking`

Source: [tutorial_benchmarking.cpp](tutorial_benchmarking.cpp).

```sh
cmake --build build-release --target tutorial_benchmarking
./build-release/tutorial/tutorial_benchmarking --lengths 64,256 --iterations 100,300 --samples 3 --warmups 1 --concurrency 0 --verbosity 4 --summary min,max,mean,mad
```

This first example deliberately demonstrates the richer framework features:
typed parameter sequences and their Cartesian product, global Threading CLI
options, Runner-managed warm-ups, separate preparation/timing/verification,
selectable summary statistics, dynamic-width report columns, numeric alignment,
table rules and configuration grouping.

Every extra benchmark argument accepts one value or a comma-separated ordered
list; duplicates are retained, with the last registered axis changing fastest.
Warm-ups default to zero and do not contribute to the measured samples. The
default `--summary` selection is `min,max,mean,mad`, with `median,p95,stddev,
variance,count` also available. Columns are sized using their longest
fully formatted values.

Verbosity 0 suppresses benchmark messages. Verbosity 1 prints the parameter
sequences and final summary; 2 adds configuration lifecycle events in a matrix;
3 adds execution lifecycle events; 4 adds individual sample timings. With a
single configuration, execution and measurement details each move one level
closer to the summary.

## 2. Minimal comparison: `tutorial_comparison`

Source: [tutorial_comparison.cpp](tutorial_comparison.cpp).

```sh
cmake --build build-release --target tutorial_comparison
./build-release/tutorial/tutorial_comparison --verbosity 1
```

Only the essential A/B concepts appear here: two concrete implementation types
(`VariantA` and `VariantB`), one equivalent operation, one fixed configuration,
one reference (`A`) and a single paired report showing the candidate/reference
slowdown. The templated workload compiles separately for both implementations,
with no virtual dispatch inside the timed operation. Values above one mean
the candidate is slower.

More advanced N-way comparisons, multiple operations, parameter matrices and
independently configured measurement/pair/aggregate reports are supported by
the framework and tested separately. They do not complicate the introductory
A/B tutorial.
