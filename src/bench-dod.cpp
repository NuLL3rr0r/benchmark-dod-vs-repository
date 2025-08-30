#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <print>
#include <random>
#include <string>
#include <vector>

#include "lib.hpp"

struct UsersView
{
    const int32_t* RESTRICT_ALIAS Ids;
    const float* RESTRICT_ALIAS Balances;
    const uint8_t* RESTRICT_ALIAS Active;
    std::size_t Count;
};

FORCE_NOINLINE float SumActiveBalances(
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

int32_t main()
{
    constexpr std::size_t elementsCount = 10'000'000;
    constexpr float minimumBalance = 250.0f;
    constexpr uint_fast32_t randomSeed = 17;
    constexpr std::size_t warmupIterations = 2;
    constexpr std::size_t iterations = 8;

    std::println("");
    std::println("[ DoD Benchmark ]");
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
    std::println("[ DoD Results ]");
    std::println("Checksum                   : {:.8f}", checksum);
    std::println("Total Time                 : {:.2f} s", totalTimeSeconds);
    std::println("Average Time per Iteration : {:.2f} s", averageTimeSeconds);
    std::println("Elements per Second        : {:.2f} M", elementsPerSecond / 1e6);
    std::println("Nanoseconds per Element    : {:.2f}", nanosecondsPerElement);
    std::println("");

    return EXIT_SUCCESS;
}
