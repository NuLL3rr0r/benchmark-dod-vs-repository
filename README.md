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

## Clone & Build

```bash
$ git clone https://github.com/NuLL3rr0r/benchmark-dod-vs-repository.git
$ cd benchmark-dod-vs-repository
$ make
```
