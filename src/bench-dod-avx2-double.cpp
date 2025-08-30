#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <print>
#include <random>
#include <string>
#include <vector>

#include <immintrin.h>

#include "lib.hpp"

struct UsersView
{
    const int32_t* RESTRICT_ALIAS Ids;
    const float* RESTRICT_ALIAS Balances;
    const uint8_t* RESTRICT_ALIAS Active;
    std::size_t Count;
};

FORCE_NOINLINE float SumActiveBalancesScalar(
    const UsersView &usersView, const float minimumBalance)
{
    float accumulatedBalance = 0.0f;
    const float thresholdBalance = minimumBalance;

    for (std::size_t i = 0; i < usersView.Count; ++i) {
        const float balanceValue = usersView.Balances[i];
        const float takeValue =
            (usersView.Active[i] && balanceValue >= thresholdBalance)
                ? 1.0f : 0.0f;
        accumulatedBalance += balanceValue * takeValue;
    }

    return accumulatedBalance;
}

#if defined(__AVX2__)
FORCE_NOINLINE double SumActiveBalancesAvx2(
    const UsersView& usersView, float minimumBalance)
{
    const std::size_t count = usersView.Count;
    const float* RESTRICT_ALIAS balances = usersView.Balances;
    const std::uint8_t* RESTRICT_ALIAS activeFlags = usersView.Active;

    const __m256 threshold = _mm256_set1_ps(minimumBalance);
    const __m256 one = _mm256_set1_ps(1.0f);

    __m256d acc0 = _mm256_setzero_pd();
    __m256d acc1 = _mm256_setzero_pd();

    constexpr std::size_t vectorWidth = 8;
    const std::size_t n8 = (count / vectorWidth) * vectorWidth;

    std::size_t i = 0;
    for (; i < n8; i += vectorWidth) {
        __m256 b = _mm256_loadu_ps(&balances[i]);
        __m128i bytes =
            _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&activeFlags[i]));
        __m256i ints = _mm256_cvtepu8_epi32(bytes);
        __m256 activeM = _mm256_cvtepi32_ps(ints);
        activeM = _mm256_min_ps(activeM, one);

        __m256 cmpMask = _mm256_cmp_ps(b, threshold, _CMP_GE_OQ);
        __m256 take = _mm256_and_ps(cmpMask, activeM);
        __m256 contrib = _mm256_mul_ps(b, take);

        __m128 low = _mm256_castps256_ps128(contrib);
        __m128 high = _mm256_extractf128_ps(contrib, 1);

        acc0 = _mm256_add_pd(acc0, _mm256_cvtps_pd(low));
        acc1 = _mm256_add_pd(acc1, _mm256_cvtps_pd(high));
    }

    __m256d acc = _mm256_add_pd(acc0, acc1);
    __m128d low = _mm256_castpd256_pd128(acc);
    __m128d high = _mm256_extractf128_pd(acc, 1);
    __m128d sum = _mm_add_pd(low, high);
    double accumulatedBalance =
        _mm_cvtsd_f64(sum) + _mm_cvtsd_f64(_mm_unpackhi_pd(sum, sum));

    for (; i < count; ++i) {
        if (activeFlags[i] && balances[i] >= minimumBalance) {
            accumulatedBalance += static_cast<double>(balances[i]);
        }
    }

    return accumulatedBalance;
}
#endif  /* defined(__AVX2__) */

FORCE_NOINLINE float SumActiveBalances(
    const UsersView &usersView, const float minimumBalance)
{
#if defined(__AVX2__)
#if COMPILER_CLANG || COMPILER_GCC
    if (__builtin_cpu_supports("avx2")) {
        return SumActiveBalancesAvx2(usersView, minimumBalance);
    }
#endif  /* COMPILER_CLANG || COMPILER_GCC */
    return SumActiveBalancesScalar(usersView, minimumBalance);
#else  /* defined(__AVX2__) */
    return SumActiveBalancesScalar(usersView, minimumBalance);
#endif  /* defined(__AVX2__) */
}

int32_t main()
{
    constexpr std::size_t elementsCount = 1'000'000'000;
    constexpr float minimumBalance = 250.0f;
    constexpr uint_fast32_t randomSeed = 17;
    constexpr std::size_t warmupIterations = 2;
    constexpr std::size_t iterations = 8;

    std::println("");
    std::println("[ DoD AVX2 Benchmark ]");
    std::println("Elements Count    : {}", elementsCount);
    std::println("Minimum Balance   : {:.2f}", minimumBalance);
    std::println("Random Seed       : {}", randomSeed);
    std::println("Warmup Iterations : {}", warmupIterations);
    std::println("Iterations        : {}", iterations);

    std::mt19937 randomEngine{randomSeed};
    std::uniform_real_distribution<float> balanceDistribution{0.0f, 1000.0f};
    std::bernoulli_distribution           activeDistribution{0.6};

    std::println("");
    std::println("Generating elements...");

    std::vector<std::int32_t> userIds(elementsCount);
    std::vector<float> userBalances(elementsCount);
    std::vector<std::uint8_t> userActiveFlags(elementsCount);

    for (std::size_t i = 0; i < elementsCount; ++i) {
        userIds[i] = static_cast<std::int32_t>(i);
        userBalances[i] = balanceDistribution(randomEngine);
        userActiveFlags[i] = activeDistribution(randomEngine) ? 1u : 0u;
    }

    UsersView usersView{
        userIds.data(),
        userBalances.data(),
        userActiveFlags.data(),
        elementsCount,
    };

    std::println("");
    std::println("Warming up...");

    double checksum = 0.0;
    for (std::size_t i = 0; i < warmupIterations; ++i) {
        checksum = SumActiveBalances(usersView, minimumBalance);
    }

    std::println("");
    std::println("Benchmarking...");

    const double totalTimeSeconds = MeasureExecutionTime(
        iterations, [&] {
            return SumActiveBalances(usersView, minimumBalance);
        });

    const double averageTimeSeconds = totalTimeSeconds / iterations;
    const double elementsPerSecond =
        static_cast<double>(elementsCount) / averageTimeSeconds;
    const double nanosecondsPerElement =
        (averageTimeSeconds * 1e9) / static_cast<double>(elementsCount);

    std::println("");
    std::println("[ DoD AVX2 Results ]");
    std::println("Checksum                   : {:.8f}", checksum);
    std::println("Total Time                 : {:.2f} s", totalTimeSeconds);
    std::println("Average Time per Iteration : {:.2f} s", averageTimeSeconds);
    std::println("Elements per Second        : {:.2f} M", elementsPerSecond / 1e6);
    std::println("Nanoseconds per Element    : {:.2f}", nanosecondsPerElement);
    std::println("");

    return EXIT_SUCCESS;
}
