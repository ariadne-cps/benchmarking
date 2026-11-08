# C++ tutorial

One source file, one executable: [tutorial_benchmarking.cpp](tutorial_benchmarking.cpp).

```sh
cmake --build build-release --target tutorial_benchmarking
./build-release/tutorial/tutorial_benchmarking --lengths 64,256 --iterations 100,300 --samples 3 --warmups 1 --concurrency 0 --verbosity 4 --summary min,max,mean,mad
```

## Part I — ordinary benchmark

The first part demonstrates the more sophisticated features: typed parameter
sequences, Cartesian combinations, global concurrency and Logging options,
Runner-managed warm-ups (default zero), explicit input preparation and
correctness checks outside the timer, configurable summary statistics, dynamic
column widths, numerical alignment, table rules and grouping.

The default `--summary` is `min,max,mean,mad`; alternatives include
`median,p95,stddev,variance,count`. Parameter values may be supplied as ordered
comma-separated lists and duplicates are preserved. Warm-ups are checked but
not included in statistics.

Logging levels are 1 for the summary, 2 for configuration lifecycle, 3 for
per-configuration execution lifecycle and 4 for individual measurements in a
matrix. With one configuration, execution detail starts at level 2.

## Part II — minimal A/B comparison

The second part executes automatically, after the ordinary benchmark, in the
**same executable** and with the same globally configured CLI options.
It uses only two implementation types (`VariantA` and `VariantB`), one
operation, one fixed configuration, and one report showing the slowdown
of candidate B relative to reference A. The workload is instantiated at
compile time per implementation; no virtual dispatch is used by the measured
operation.

The example is synthetic, not a scientific comparison between real arithmetic
backends. General N-way comparisons and independently configurable raw, paired
and aggregate reports are tested in the framework but intentionally omitted
from the introductory A/B portion.
