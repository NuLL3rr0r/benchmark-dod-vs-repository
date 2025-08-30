# C++ Repository Pattern vs Data-Oriented Design Benchmark

A small C++ project comparing the __Repository Pattern__ (object-oriented, callback-driven design) against __Data-Oriented Design__ (SoA layout, hot-loop friendly) for a simple filtering/summing workload.

The aim of this benchmark is to __make performance trade-offs tangible__:
- Repository Pattern is often considered "clean" or "maintainable" but hides the hot loop behind virtual dispatch and type-erased callbacks.
- Data-Oriented Design keeps data in flat arrays, exposes aliasing guarantees (`restrict`), and enables the compiler to optimize with vectorization (SSE/AVX2/FMA).

This project provides minimal examples of both styles and measures their performance on large datasets.

## Benchmarked Programs

- __`bench-repository`__: A __Repository Pattern__ version using an abstract interface, virtual dispatch, and callbacks. Demonstrates the cost of abstraction: harder for the compiler to inline and vectorize the hot loop.

- __`bench-dod`__: Scalar baseline using a flat, __struct-of-arrays (SoA)__ data layout. Demonstrates how separation of hot data enables compiler optimizations.

- __`bench-dod-avx2`__: Vectorized SoA implementation using __AVX2/FMA intrinsics__. Processes 8 elements per iteration with SIMD, plus a scalar remainder path.

- __`bench-dod-znver2`__: Hand-tuned AVX2/FMA path optimized for __AMD Zen 2 (e.g. Threadripper 3960X)__. Uses dual accumulators for ILP and light prefetching, sustaining higher throughput on Zen 2’s dual FMA units.

Each benchmark also has a `-double` variant that uses __double precision accumulation__.

- Float versions are benchmarked at `10` million records.
- Double versions scale up to `1` billion records without overflow and less potential drift in the SIMD variants.

## Sample Benchmark Results

Here are some sample outputs from the benchmarked programs on my `AMD Ryzen Threadripper 3960X`:

```sh
$ ./bin/bench-repository

[ Repository Benchmark ]
Elements Count    : 10000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ Repository Results ]
Checksum                   : 2809819648.00000000
Total Time                 : 0.47 s
Average Time per Iteration : 0.06 s
Elements per Second        : 170.23 M
Nanoseconds per Element    : 5.87
```

```sh
$ ./bin/bench-dod

[ DoD Benchmark ]
Elements Count    : 10000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ DoD Results ]
Checksum                   : 2809819648.00000000
Total Time                 : 0.23 s
Average Time per Iteration : 0.03 s
Elements per Second        : 354.29 M
Nanoseconds per Element    : 2.82
```

```sh
$ ./bin/bench-dod-avx2

[ DoD AVX2 Benchmark ]
Elements Count    : 10000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ DoD AVX2 Results ]
Checksum                   : 2811114496.00000000
Total Time                 : 0.02 s
Average Time per Iteration : 0.00 s
Elements per Second        : 4436.42 M
Nanoseconds per Element    : 0.23
```

```sh
$ ./bin/bench-dod-znver2

[ DoD Znver2 Benchmark ]
Elements Count    : 10000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ DoD Znver2 Results ]
Checksum                   : 2811144960.00000000
Total Time                 : 0.02 s
Average Time per Iteration : 0.00 s
Elements per Second        : 4682.48 M
Nanoseconds per Element    : 0.21
```

```sh
$ ./bin/bench-repository-double

[ Repository Benchmark ]
Elements Count    : 1000000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ Repository Results ]
Checksum                   : 281261069117.34124756
Total Time                 : 46.85 s
Average Time per Iteration : 5.86 s
Elements per Second        : 170.77 M
Nanoseconds per Element    : 5.86
```

```sh
$ ./bin/bench-dod-double

[ DoD Benchmark ]
Elements Count    : 1000000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ DoD Results ]
Checksum                   : 281261069117.34124756
Total Time                 : 39.27 s
Average Time per Iteration : 4.91 s
Elements per Second        : 203.74 M
Nanoseconds per Element    : 4.91
```

```sh
$ ./bin/bench-dod-avx2-double

[ DoD AVX2 Benchmark ]
Elements Count    : 1000000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ DoD AVX2 Results ]
Checksum                   : 281261080576.00000000
Total Time                 : 2.20 s
Average Time per Iteration : 0.28 s
Elements per Second        : 3628.55 M
Nanoseconds per Element    : 0.28
```

```sh
$ ./bin/bench-dod-znver2-double

[ DoD Znver2 Benchmark ]
Elements Count    : 1000000000
Minimum Balance   : 250.00
Random Seed       : 17
Warmup Iterations : 2
Iterations        : 8

Generating elements...

Warming up...

Benchmarking...

[ DoD Znver2 Results ]
Checksum                   : 281261080576.00000000
Total Time                 : 1.93 s
Average Time per Iteration : 0.24 s
Elements per Second        : 4152.39 M
Nanoseconds per Element    : 0.24
```

__Note__: If you use the same `Random Seed` in all programs, the exact same sequence of balances and active flags will be generated, which is proof that we ran the benchmarked programs against the same dataset. You can verify this by comparing the reported `Checksum` values from the output. However, since we calculate the checksum during the warmup phase, the reported checksums differ in the AVX2/Znver2 versions. Why? Because the SIMD implementations use vectorized reductions and fused multiply-add (FMA) instructions, which change the order of additions. Since floating-point addition is not associative, these small changes in evaluation order lead to different rounding and thus slightly different checksums. This is expected and does not affect the validity of the performance comparison.

## Clone & Build

```bash
$ git clone https://github.com/NuLL3rr0r/benchmark-dod-vs-repository.git
$ cd benchmark-dod-vs-repository
$ make
```
