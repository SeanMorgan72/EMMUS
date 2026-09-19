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

TEST(BenchmarkConfigurationIntegrationTest, SeededBenchmarkReproducesSimulationAcrossRuns)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        2,
        8,
        60,
        WorkloadType::Random,
        4242,
        PageReplacementPolicyType::FIFO);

    const auto firstConfiguration = benchmark.toSimulationConfiguration();
    const auto secondConfiguration = benchmark.toSimulationConfiguration();

    const auto firstResult = Simulation(firstConfiguration).run();
    const auto secondResult = Simulation(secondConfiguration).run();

    EXPECT_EQ(
        firstResult.executionResult().records().size(),
        secondResult.executionResult().records().size());
    EXPECT_EQ(
        firstResult.executionResult().statistics().pageFaultCount(),
        secondResult.executionResult().statistics().pageFaultCount());
    EXPECT_EQ(
        firstResult.pageReplacementStatistics().replacementCount(),
        secondResult.pageReplacementStatistics().replacementCount());
}

TEST(BenchmarkConfigurationIntegrationTest, ComparisonBenchmarkChangesOnlyReplacementPolicy)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{3},
        2,
        6,
        50,
        WorkloadType::Random,
        99,
        PageReplacementPolicyType::FIFO);

    const auto fifoConfiguration = benchmark.toSimulationConfiguration();
    const auto lruConfiguration =
        benchmark.withReplacementPolicy(PageReplacementPolicyType::LRU)
            .toSimulationConfiguration();

    EXPECT_EQ(fifoConfiguration.pageSize(), lruConfiguration.pageSize());
    EXPECT_EQ(fifoConfiguration.frameCount(), lruConfiguration.frameCount());
    EXPECT_EQ(fifoConfiguration.processCount(), lruConfiguration.processCount());
    EXPECT_EQ(
        fifoConfiguration.pageCountPerProcess(),
        lruConfiguration.pageCountPerProcess());
    EXPECT_EQ(
        fifoConfiguration.memoryAccessCount(),
        lruConfiguration.memoryAccessCount());
    EXPECT_EQ(fifoConfiguration.randomSeed(), lruConfiguration.randomSeed());
    EXPECT_NE(
        fifoConfiguration.replacementPolicy(),
        lruConfiguration.replacementPolicy());
}

TEST(BenchmarkConfigurationIntegrationTest, InvalidBenchmarkIsRejectedBeforeSimulation)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        0,
        6,
        30,
        WorkloadType::Random,
        10,
        PageReplacementPolicyType::FIFO);

    EXPECT_FALSE(benchmark.isValid());
    EXPECT_FALSE(benchmark.validate().empty());
    EXPECT_THROW(benchmark.toSimulationConfiguration(), std::invalid_argument);
}
