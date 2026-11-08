# C++ benchmarking tutorial

The example in [tutorial_benchmarking.cpp](tutorial_benchmarking.cpp) is a
single C++20 executable demonstrating **two independent experiments**, run in
sequence: a parameterized benchmark of a sum, and a minimal A/B comparison of
two concrete implementation types. The experiments exercise the same reusable
framework but have different configuration and reporting needs.

## Build and run

From the repository root, after initializing submodules:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target tutorial_benchmarking
./build-release/tutorial/tutorial_benchmarking --lengths 64,256 --iterations 100,300 --samples 3 --warmups 1 --concurrency 0 --verbosity 4 --summary min,max,mean,mad
```

Use a Release build for meaningful comparisons: a Debug build can change both
the amount of generated work and the optimizer's treatment of the loop.
Even in Release, tiny times vary with machine load, CPU frequency, cache
effects and compiler optimizations. The example demonstrates **the framework**,
not a reproducible ranking of arithmetic algorithms.

`--lengths`, `--iterations` and `--samples` control **Part I**.
`--warmups`, `--summary`, threading and Logging options are registered
globally by `Bench::CommandLine`. The A/B experiment in Part II uses its own
fixed five measured rounds per backend, while still respecting the global
warm-up policy. It does not use Part I's `--samples` parameter or matrix.

## Part I: parameterized, validated timing

### 1. Create typed experiment axes

```cpp
Bench::Parameter<unsigned> lengths("lengths", "Input lengths", {64u,256u});
Bench::Parameter<unsigned> iterations("iterations", "Iterations", {1000u});
Bench::Parameter<unsigned> samples_count("samples", "Samples", {7u});
Bench::Matrix matrix;
matrix.add(lengths).add(iterations).add(samples_count);
```

Each `Parameter<T>` has a name used as a command-line option, help text and
an ordered list of typed default values. The CLI can replace a list with a
single value or a comma-separated sequence. Duplicates are not discarded:
they represent separate positions in the Cartesian product.

`Matrix` **borrows** the axes; the parameter objects must outlive the matrix
iteration and CLI acquisition. Its `for_each` visits combinations lazily,
rather than constructing an in-memory array of all configurations. The last
registered axis changes fastest.

With `--lengths 64,256 --iterations 100,300 --samples 3`, the axes contain
2, 2 and 1 values, resulting in `2 * 2 * 1 = 4` configurations. The order is
(64,100,3), (64,300,3), (256,100,3), (256,300,3).
The number of configurations is **not** the number of measured samples.
A `Configuration` exposes its values through `config.value(axis)`, as well
as its one-based ordinal and a readable dimension label.

### 2. Parse the global and experiment-specific options

```cpp
Bench::CommandLine cli;
cli.add_parameters(matrix);
if (not cli.acquire(Bench::arguments(argc, argv))) return 0;
```

`CommandLine` extends the common Threading CLI and supports Logging,
`--concurrency`, `--summary` and `--warmups`. Registering the matrix
adds its typed parameters to the same parser. The `acquire` call processes
all settings before any measurements begin; the early return handles
non-execution CLI requests (such as help).

The default summary selection is `min,max,mean,mad`. Other available
identifiers are `median,p95,stddev,variance,count`. The requested field order
is retained and duplicate identifiers are rejected.

### 3. Configure a human-readable report

`Bench::Report` receives the requested summary fields and four **descriptive**
columns: Length, Iterations, Samples and Operation. These are distinct from the
numeric statistics columns appended by the report. Right-aligned numeric
descriptors make different input sizes easier to scan.

`Bench::TableOptions::ruled()` enables top, header, group and bottom rules.
`group_columns = {0u}` inserts a rule whenever the first descriptive column
(Length) changes between adjacent result rows. The framework computes widths
from the rendered headings and cell contents; it does not truncate larger
values to a fixed column width.

`report.set_parameter_header(matrix.header())` adds a concise declaration of
the experiment axes and total number of configurations. The report formats
statistics after timing has finished; it does not measure any operations.

### 4. Separate input setup, timed work and validation

For every configuration, the tutorial allocates a `std::vector<std::uint64_t>`
of the selected length and fills it with the integers 1 through `size`.
It also computes `expected`, the sum of those values, **outside timing**.
Lengths, repetitions and measured sample counts must all be positive.

The central call uses three explicit callbacks:

```cpp
Bench::Samples results = runner.run(
    "sum", measured,
    [&] { sum = 0u; },
    [&] {
        for (unsigned k = 0u; k < repetitions; ++k) {
            for (auto const value : data) sum += value;
            Bench::keep_alive(sum);
        }
    },
    [&] {
        if (sum != expected * repetitions)
            throw std::runtime_error("incorrect sum");
    },
    static_cast<double>(repetitions));
```

The exact execution sequence of **one round** is:

1. `prepare` resets `sum` to zero outside the timed region.
2. `operation` traverses the entire vector `repetitions` times, with only
   this callback measured by a monotonic `steady_clock`.
3. `keep_alive(sum)` makes the current accumulator observable to the
   compiler, discouraging removal of apparently unused work.
4. `verify` checks the final accumulated sum outside timing. An incorrect
   result throws and terminates the experiment rather than producing a
   misleading performance row.

The input vector is prepared once per **configuration**. The accumulator is
reset before **every execution**, including warm-ups, so measurements are not
dependent on the previous round's accumulated result. The arithmetic uses
unsigned 64-bit values; this teaching example is intended for modest input
sizes, not for exploring overflowing sums.

### 5. Distinguish warm-ups, samples and normalization

`Bench::Runner runner(cli, matrix.combinations())` reads the global
`--warmups` setting and enables matrix-aware progress messages.
For `--warmups 1 --samples 3`, each Part I configuration executes
**four** complete rounds: one warm-up and three measured rounds. The
warm-up is also prepared, timed and verified, but its duration is discarded.
`runner.run` therefore returns exactly three values.

A full timed execution performs `repetitions` traversals of the vector.
The last argument, `static_cast<double>(repetitions)`, **divides each full
timed duration by the repetition count**. Therefore reported values are
**nanoseconds per traversal of the selected vector**, *not* nanoseconds
per individual element or per measured round. For example, if a timed run
of 100 traversals takes 20,000 ns, its normalized sample is 200 ns per
traversal. Preparation, verification and reporting are not included.

The statistics are computed from these normalized samples.
`Bench::summarize` calculates minimum, maximum, arithmetic mean, median,
median absolute deviation (MAD), nearest-rank 95th percentile, and sample
variance/standard deviation (using an `n-1` denominator). For a single
sample, variance and standard deviation are undefined and display as
`n/a`. Variance has units ns² rather than ns.

Finally, each result is added as a report row with the four descriptor values.
The report prints once after all matrix configurations have completed.

### 6. Choose progress detail with Logging

The reporting summary uses verbosity 1. With multiple configurations,
verbosity 2 adds configuration starts/completions, 3 adds execution
starts/completions, and 4 adds individual measured durations. When there is
only one configuration, execution and measurement detail shift down to
levels 2 and 3, because there is no matrix lifecycle to expand.

Warm-up executions appear in lifecycle logs, but are excluded from the
individual measurement values and final statistical summaries. Higher
verbosity makes work observable; it does not change the sampling contract.

## Part II: minimal type-based A/B comparison

### 1. Define implementation types

`VariantA` and `VariantB` each provide
`static unsigned increment(unsigned)`. Their two expressions,
`value + 1u` and `1u + value`, have equivalent arithmetic semantics;
compilers may generate identical instructions. These are deliberately
**synthetic** backends, not credible alternatives for a performance claim.

### 2. Build a fixed experiment and battery

```cpp
Bench::Matrix one_configuration;
Bench::Runner runner(cli);
Bench::ComparisonBattery<VariantA, VariantB> battery({"A", "B"});
```

An empty `Matrix` still has **one** implicit configuration.
`ComparisonBattery<VariantA,VariantB>` instantiates the templated workload
separately for each concrete type: the measured operation does not select a
backend using runtime virtual dispatch. The battery accumulates measurements
under the labels `A` and `B`.

### 3. Register the same operation for each type

The generic lambda receives a `record` callback and invokes it with the
operation name `"increment"`, preparation, timed operation, verification and
normalization. Preparation resets `value` to zero. The timed callback runs
1,000 increments, calling `keep_alive` on the result. Verification checks
that the final value is exactly 1,000.

Normalization `1000.0` reports **nanoseconds per increment**, not time for
all 1,000 increments. `battery.run(one_configuration, runner, 5u, workload)`
runs five measured samples for each implementation, in addition to any
globally configured warm-ups. The two implementations are run sequentially,
not interleaved or statistically randomized.

### 4. Produce a paired report

```cpp
Bench::ComparisonReports reports;
reports.add({"A/B", Bench::ComparisonReportKind::Pairs,
             {"operation", "reference_ns", "candidate_ns", "slowdown"}});
reports.print_all(battery.results(), "A");
```

The pair view matches observations by configuration ordinal and operation.
It selects the **median** timing by default, because no other location
metric is specified in this report definition. Therefore `--summary`
controls the Part I summary columns, not this A/B report's columns or
pairing metric.

The displayed slowdown is

```text
slowdown = candidate_ns / reference_ns
```

where `A` is the reference and `B` is the candidate. A slowdown greater
than 1 means B is **slower** than A; less than 1 means B is faster.
The reciprocal quantity `speedup = reference_ns / candidate_ns` is also
available in paired report views. Invalid, nonpositive or unmatched timings
are rejected instead of being silently compared.

This introductory A/B example purposely omits the general N-way battery,
multiple operations, raw measurement tables and aggregate reports.
`ComparisonReports` can independently format raw, paired and aggregate
views; aggregate slowdown includes a geometric mean and a ratio of total
times. The library tests cover those broader cases.

## Interpreting results responsibly

The framework eliminates some common benchmarking errors, but does not
guarantee fair experimental conditions. Run repeated experiments under
comparable CPU frequency, load, build flags and input distributions. Be
especially cautious with synthetic examples: constant-sized computations
may be optimized differently, and sub-nanosecond normalized numbers may
reflect a longer loop divided by its iteration count rather than an
individually observable instruction latency.

This tutorial demonstrates how to **structure** a benchmark: control its
inputs, measure only the intended code, check correctness, normalize the
measurement, summarize the distribution and present comparisons with
explicit direction and units. It does not establish that VariantA or
VariantB is a faster implementation.
