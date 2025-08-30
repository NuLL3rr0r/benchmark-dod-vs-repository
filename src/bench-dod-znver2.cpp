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
FORCE_NOINLINE float SumActiveBalancesAvx2(
    const UsersView& usersView, float minimumBalance)
{
    const std::size_t count = usersView.Count;
    const float* RESTRICT_ALIAS balances = usersView.Balances;
    const std::uint8_t* RESTRICT_ALIAS activeFlags = usersView.Active;

    const __m256 threshold = _mm256_set1_ps(minimumBalance);
    const __m256 one = _mm256_set1_ps(1.0f);

    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();

    constexpr int32_t prefetchDistance = 256;

    constexpr std::size_t vectorWidth = 16;
    const std::size_t n16 = (count / vectorWidth) * vectorWidth;

    std::size_t i = 0;
    for (; i < n16; i += vectorWidth) {
        _mm_prefetch(reinterpret_cast<const char*>(balances + i) + prefetchDistance, _MM_HINT_T0);
        _mm_prefetch(reinterpret_cast<const char*>(activeFlags + i) + prefetchDistance, _MM_HINT_T0);

        __m256 b0 = _mm256_loadu_ps(balances + i);
        __m128i a8_0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(activeFlags + i));
        __m256i a32_0 = _mm256_cvtepu8_epi32(a8_0);
        __m256 active0 = _mm256_min_ps(_mm256_cvtepi32_ps(a32_0), one);

        __m256 cmp0 = _mm256_cmp_ps(b0, threshold, _CMP_GE_OQ);
        __m256 contrib0 = _mm256_mul_ps(b0, _mm256_and_ps(cmp0, active0));

        acc0 = _mm256_add_ps(acc0, contrib0);

        __m256 b1 = _mm256_loadu_ps(balances + i + 8);
        __m128i a8_1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(activeFlags + i + 8));
        __m256i a32_1 = _mm256_cvtepu8_epi32(a8_1);
        __m256 active1 = _mm256_min_ps(_mm256_cvtepi32_ps(a32_1), one);

        __m256 cmp1 = _mm256_cmp_ps(b1, threshold, _CMP_GE_OQ);
        __m256 contrib1 = _mm256_mul_ps(b1, _mm256_and_ps(cmp1, active1));

        acc1 = _mm256_add_ps(acc1, contrib1);
    }

    __m256 acc = _mm256_add_ps(acc0, acc1);
    __m128 low = _mm256_castps256_ps128(acc);
    __m128 high = _mm256_extractf128_ps(acc, 1);
    __m128 sum = _mm_add_ps(low, high);
    sum = _mm_hadd_ps(sum, sum);
    sum = _mm_hadd_ps(sum, sum);

    float accumulatedBalance = _mm_cvtss_f32(sum);

    for (; i < count; ++i) {
        if (activeFlags[i] && balances[i] >= minimumBalance) {
            accumulatedBalance += balances[i];
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
    constexpr std::size_t elementsCount = 10'000'000;
    constexpr float minimumBalance = 250.0f;
    constexpr uint_fast32_t randomSeed = 17;
    constexpr std::size_t warmupIterations = 2;
    constexpr std::size_t iterations = 8;

    std::println("");
    std::println("[ DoD Znver2 Benchmark ]");
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

    float checksum = 0.0f;
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
    std::println("[ DoD Znver2 Results ]");
    std::println("Checksum                   : {:.8f}", checksum);
    std::println("Total Time                 : {:.2f} s", totalTimeSeconds);
    std::println("Average Time per Iteration : {:.2f} s", averageTimeSeconds);
    std::println("Elements per Second        : {:.2f} M", elementsPerSecond / 1e6);
    std::println("Nanoseconds per Element    : {:.2f}", nanosecondsPerElement);
    std::println("");

    return EXIT_SUCCESS;
}
