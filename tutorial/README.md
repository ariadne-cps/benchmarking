# C++ benchmarking tutorial

The maintained `tutorial_benchmarking.cpp` demonstrates parameter sequences,
Cartesian combinations and progressively detailed Logging.

```sh
cmake --build build-release --target tutorial_benchmarking
./build-release/tutorial/tutorial_benchmarking --lengths 64,256 --iterations 100,300 --samples 3 --warmups 1 --concurrency 0 --verbosity 4
```

All benchmark-specific arguments accept either a scalar or a comma-separated
list; duplicates are preserved and the final registered axis varies fastest.
Global `--concurrency`, `--warmups` and Logging options are not experiment axes.

Verbosity 0: silent. Verbosity 1: sequences and overall results. Verbosity
2: matrix configuration start/complete (or execution start/complete for a
single configuration). Verbosity 3: matrix execution start/complete (or
measurements for a single configuration). Verbosity 4: individual matrix
measurements. The measurement loop itself does not log.

`Runner` performs all warm-ups before measured samples in each configuration,
with `--warmups 0` as the default. Each warm-up calls the same prepare,
operation and verify callbacks as measured rounds; it never contributes to
statistics. Each benchmark supplies those three callbacks, not its own
warm-up loop. The tutorial keeps input construction outside all timed scopes.

## Selecting summary columns

Use `--summary min,max,mean,mad` (the default) or a comma-separated
selection such as `--summary median,mad,stddev,p95,count`. The output
preserves the requested column order. Available names are `min`, `max`,
`mean`, `mad`, `median`, `p95`, `stddev`, `variance`, and `count`.

All timing columns are in nanoseconds except `variance`, which is in
squared nanoseconds. Variance and standard deviation are sample estimates
(denominator N-1). For a single sample these fields display `n/a`, not zero.
The tutorial passes `cli.summary_fields()` to the `Report` constructor;
`--summary` only controls the final summary columns, not log verbosity,
sampling or benchmark parameter axes.

For a benchmark with two warm-ups per configuration, pass `--warmups 2`.
Omit the option to disable warm-ups entirely.
