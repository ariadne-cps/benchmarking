# C++ benchmarking tutorial

The maintained `tutorial_benchmarking.cpp` demonstrates parameter sequences,
Cartesian combinations and progressively detailed Logging.

```sh
cmake --build build-release --target tutorial_benchmarking
./build-release/tutorial/tutorial_benchmarking --lengths 64,256 --iterations 100,300 --samples 3 --warmups 1 --concurrency 0 --verbosity 4
```

All benchmark-specific arguments accept either a scalar or a comma-separated
list; duplicates are preserved and the final registered axis varies fastest.
Global `--concurrency` and Logging options are not experiment axes.

Verbosity 0: silent. Verbosity 1: sequences and overall results. Verbosity
2: matrix configuration start/complete (or execution start/complete for a
single configuration). Verbosity 3: matrix execution start/complete (or
measurements for a single configuration). Verbosity 4: individual matrix
measurements. The measurement loop itself does not log.

Construction, warmup, timing, correctness checks and reporting are explicit.
