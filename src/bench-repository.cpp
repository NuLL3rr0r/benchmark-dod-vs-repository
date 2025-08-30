#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "lib.hpp"

struct User
{
    int32_t Id;
    float Balance;
    bool Active;
};

struct IUserRepository
{
    virtual ~IUserRepository() = default;
    virtual void ForEach(const std::function<void(const User&)>& fn) const = 0;
    virtual std::optional<User> FindById(int32_t id) const = 0;
};

class VectorUserRepository final : public IUserRepository
{
public:
    explicit VectorUserRepository(const std::vector<User>& users)
        : Users(users)
    {
    }

    explicit VectorUserRepository(std::vector<User>&& users) noexcept
        : Users(std::move(users))
    {
    }

    void ForEach(const std::function<void(const User&)>& fn) const override
    {
        for (const User& user : Users) {
            fn(user);
        }
    }

    std::optional<User> FindById(const int32_t id) const override
    {
        for (const User& user : Users) {
            if (user.Id == id) {
                return user;
            }
        }

        return std::nullopt;
    }

private:
    std::vector<User> Users;
};

[[nodiscard]] bool Qualifies(const User& user, const float minimumBalance)
{
    const bool bQualifies = user.Active && user.Balance >= minimumBalance;
    return bQualifies;
}

FORCE_NOINLINE float SumActiveBalances(
    const IUserRepository& repository, float minimumBalance)
{
    float accumulatedBalance = 0.0f;

    repository.ForEach([&](const User& user) {
        if (Qualifies(user, minimumBalance)){
             accumulatedBalance += user.Balance;
        }
    });

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
    std::println("[ Repository Benchmark ]");
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

    std::vector<User> users;
    users.reserve(elementsCount);
    for (std::size_t i = 0; i < elementsCount; ++i) {
        User user{
            static_cast<std::int32_t>(i),
            balanceDistribution(randomEngine),
            activeDistribution(randomEngine)
        };
        users.emplace_back(std::move(user));
    }

    VectorUserRepository repository{std::move(users)};

    std::println("");
    std::println("Warming up...");

    float checksum = 0.0f;
    for (std::size_t i = 0; i < warmupIterations; ++i) {
        checksum = SumActiveBalances(repository, minimumBalance);
    }

    std::println("");
    std::println("Benchmarking...");

    const double totalTimeSeconds = MeasureExecutionTime(
        iterations, [&] {
            return SumActiveBalances(repository, minimumBalance);
        });

    const double averageTimeSeconds = totalTimeSeconds / iterations;
    const double elementsPerSecond =
        static_cast<double>(elementsCount) / averageTimeSeconds;
    const double nanosecondsPerElement =
        (averageTimeSeconds * 1e9) / static_cast<double>(elementsCount);

    std::println("");
    std::println("[ Repository Results ]");
    std::println("Checksum                   : {:.8f}", checksum);
    std::println("Total Time                 : {:.2f} s", totalTimeSeconds);
    std::println("Average Time per Iteration : {:.2f} s", averageTimeSeconds);
    std::println("Elements per Second        : {:.2f} M", elementsPerSecond / 1e6);
    std::println("Nanoseconds per Element    : {:.2f}", nanosecondsPerElement);
    std::println("");

    return EXIT_SUCCESS;
}
