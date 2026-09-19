#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"
#include "emmus/infrastructure/configuration/SimulationConfigurationValidator.hpp"

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::BenchmarkConfiguration;
using emmus::infrastructure::configuration::SimulationConfigurationValidator;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::memory::access::FrameCount;
using emmus::memory::access::PageSize;

TEST(BenchmarkConfigurationTest, StoresExplicitBenchmarkValues)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        2,
        8,
        50,
        WorkloadType::Random,
        12345,
        PageReplacementPolicyType::LRU);

    EXPECT_EQ(benchmark.pageSize().value(), 4096U);
    EXPECT_EQ(benchmark.frameCount().value(), 4U);
    EXPECT_EQ(benchmark.processCount(), 2U);
    EXPECT_EQ(benchmark.pageCountPerProcess(), 8U);
    EXPECT_EQ(benchmark.memoryAccessCount(), 50U);
    EXPECT_EQ(benchmark.workloadType(), WorkloadType::Random);
    EXPECT_EQ(benchmark.randomSeed(), 12345U);
    EXPECT_EQ(benchmark.replacementPolicy(), PageReplacementPolicyType::LRU);
}

TEST(BenchmarkConfigurationTest, ValidatesKnownGoodConfiguration)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{3},
        2,
        6,
        24,
        WorkloadType::Random,
        7,
        PageReplacementPolicyType::FIFO);

    EXPECT_TRUE(benchmark.isValid());
    EXPECT_TRUE(SimulationConfigurationValidator::isValid(
        benchmark.toSimulationConfiguration()));
    EXPECT_TRUE(benchmark.validate().empty());
}

TEST(BenchmarkConfigurationTest, RejectsInvalidBenchmarkConfiguration)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        0,
        6,
        24,
        WorkloadType::Random,
        7,
        PageReplacementPolicyType::FIFO);

    EXPECT_FALSE(benchmark.isValid());
    EXPECT_FALSE(benchmark.validate().empty());
    EXPECT_THROW(
        benchmark.toSimulationConfiguration(),
        std::invalid_argument);
}

TEST(BenchmarkConfigurationTest, ProducesDeterministicSimulationConfiguration)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        2,
        8,
        100,
        WorkloadType::Random,
        321,
        PageReplacementPolicyType::CLOCK);

    const auto first = benchmark.toSimulationConfiguration();
    const auto second = benchmark.toSimulationConfiguration();

    EXPECT_EQ(first.pageSize(), second.pageSize());
    EXPECT_EQ(first.frameCount(), second.frameCount());
    EXPECT_EQ(first.processCount(), second.processCount());
    EXPECT_EQ(first.pageCountPerProcess(), second.pageCountPerProcess());
    EXPECT_EQ(first.memoryAccessCount(), second.memoryAccessCount());
    EXPECT_EQ(first.randomSeed(), second.randomSeed());
    EXPECT_EQ(first.replacementPolicy(), second.replacementPolicy());
}

TEST(BenchmarkConfigurationTest, ReplacesOnlyThePolicyForComparison)
{
    const BenchmarkConfiguration benchmark(
        PageSize{4096},
        FrameCount{4},
        1,
        8,
        40,
        WorkloadType::Random,
        99,
        PageReplacementPolicyType::FIFO);

    const auto fifo = benchmark.toSimulationConfiguration();
    const auto lru = benchmark.withReplacementPolicy(
        PageReplacementPolicyType::LRU).toSimulationConfiguration();

    EXPECT_EQ(fifo.pageSize(), lru.pageSize());
    EXPECT_EQ(fifo.frameCount(), lru.frameCount());
    EXPECT_EQ(fifo.processCount(), lru.processCount());
    EXPECT_EQ(fifo.pageCountPerProcess(), lru.pageCountPerProcess());
    EXPECT_EQ(fifo.memoryAccessCount(), lru.memoryAccessCount());
    EXPECT_EQ(fifo.randomSeed(), lru.randomSeed());
    EXPECT_NE(fifo.replacementPolicy(), lru.replacementPolicy());
}
