# C++ tutorial

`tutorial_benchmark.cpp` is the maintained minimal example of an Ariadne
benchmark. It is built only by the explicit `tutorial_benchmark` target.
It demonstrates a procedural workflow:

1. Parse validated CLI options including `--concurrency`, initializing
   the Logger through ThreadManager.
2. Prepare deterministic inputs outside the measurement.
3. Time exactly one intended operation with `measure_ns`.
4. Validate the result outside the timed region.
5. Exclude warm-up samples.
6. Summarize the measurements and emit a Logger-backed report.

Run `cmake --build build-release --target tutorial_benchmark` and execute
`build-release/tutorial/tutorial_benchmark`. Do not accidentally add this
target to the common `benchmarks` aggregate: it is a tutorial, not a library
performance baseline.
