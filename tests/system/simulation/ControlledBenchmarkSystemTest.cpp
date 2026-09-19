#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"
#include "emmus/simulation/Simulation.hpp"

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::BenchmarkConfiguration;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::memory::access::FrameCount;
using emmus::memory::access::PageSize;
using emmus::simulation::Simulation;

TEST(ControlledBenchmarkSystemTest, EquivalentBenchmarkRunsMatchAcrossAlgorithms)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        2,
        8,
        80,
        WorkloadType::Random,
        2024,
        PageReplacementPolicyType::FIFO);

    const auto fifoResult = Simulation(
        benchmark.toSimulationConfiguration()).run();
    const auto lruResult = Simulation(
        benchmark.withReplacementPolicy(PageReplacementPolicyType::LRU)
            .toSimulationConfiguration()).run();
    const auto clockResult = Simulation(
        benchmark.withReplacementPolicy(PageReplacementPolicyType::CLOCK)
            .toSimulationConfiguration()).run();

    EXPECT_EQ(
        fifoResult.executionResult().records().size(),
        lruResult.executionResult().records().size());
    EXPECT_EQ(
        lruResult.executionResult().records().size(),
        clockResult.executionResult().records().size());
    EXPECT_EQ(
        fifoResult.executionResult().statistics().memoryAccessCount(),
        lruResult.executionResult().statistics().memoryAccessCount());
    EXPECT_EQ(
        lruResult.executionResult().statistics().memoryAccessCount(),
        clockResult.executionResult().statistics().memoryAccessCount());
}

TEST(ControlledBenchmarkSystemTest, BenchmarkConfigurationDoesNotMutateNormalSimulationBehavior)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{3},
        2,
        6,
        40,
        WorkloadType::Random,
        777,
        PageReplacementPolicyType::FIFO);

    const auto standardResult = Simulation(
        benchmark.toSimulationConfiguration()).run();
    const auto alternativeResult = Simulation(
        benchmark.withReplacementPolicy(PageReplacementPolicyType::OPTIMAL)
            .toSimulationConfiguration()).run();

    EXPECT_EQ(
        standardResult.executionResult().records().size(),
        benchmark.memoryAccessCount());
    EXPECT_EQ(
        alternativeResult.executionResult().records().size(),
        benchmark.memoryAccessCount());
    EXPECT_EQ(
        benchmark.randomSeed(),
        777U);
    EXPECT_EQ(
        benchmark.workloadType(),
        WorkloadType::Random);
}
