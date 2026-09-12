#include <cstdint>
#include <cstddef>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::SimulationConfiguration;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::memory::access::FrameCount;
using emmus::memory::access::PageSize;

TEST(SimulationConfigurationTest, StoresConfiguredValues)
{
    const SimulationConfiguration configuration(
        PageSize{4096},
        FrameCount{4},
        2,
        8,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Random,
        100,
        12345);

    EXPECT_EQ(configuration.pageSize().value(), 4096U);
    EXPECT_EQ(configuration.frameCount().value(), 4U);
    EXPECT_EQ(configuration.processCount(), 2U);
    EXPECT_EQ(configuration.pageCountPerProcess(), 8U);

    EXPECT_EQ(
        configuration.replacementPolicy(),
        PageReplacementPolicyType::FIFO);

    EXPECT_EQ(
        configuration.workloadType(),
        WorkloadType::Random);

    EXPECT_EQ(configuration.memoryAccessCount(), 100U);
    EXPECT_EQ(configuration.randomSeed(), 12345U);
}

TEST(SimulationConfigurationTest, StoresSequentialWorkloadConfiguration)
{
    const SimulationConfiguration configuration(
        PageSize{4096},
        FrameCount{4},
        1,
        4,
        PageReplacementPolicyType::LRU,
        WorkloadType::Sequential,
        50,
        98765);

    EXPECT_EQ(configuration.pageSize().value(), 4096U);
    EXPECT_EQ(configuration.frameCount().value(), 4U);
    EXPECT_EQ(configuration.processCount(), 1U);
    EXPECT_EQ(configuration.pageCountPerProcess(), 4U);

    EXPECT_EQ(
        configuration.replacementPolicy(),
        PageReplacementPolicyType::LRU);

    EXPECT_EQ(
        configuration.workloadType(),
        WorkloadType::Sequential);

    EXPECT_EQ(configuration.memoryAccessCount(), 50U);
    EXPECT_EQ(configuration.randomSeed(), 98765U);
}

TEST(SimulationConfigurationTest, PreservesOptimalReplacementPolicy)
{
    const SimulationConfiguration configuration(
        PageSize{8192},
        FrameCount{8},
        3,
        16,
        PageReplacementPolicyType::OPTIMAL,
        WorkloadType::Random,
        250,
        42);

    EXPECT_EQ(configuration.pageSize().value(), 8192U);
    EXPECT_EQ(configuration.frameCount().value(), 8U);
    EXPECT_EQ(configuration.processCount(), 3U);
    EXPECT_EQ(configuration.pageCountPerProcess(), 16U);

    EXPECT_EQ(
        configuration.replacementPolicy(),
        PageReplacementPolicyType::OPTIMAL);

    EXPECT_EQ(
        configuration.workloadType(),
        WorkloadType::Random);

    EXPECT_EQ(configuration.memoryAccessCount(), 250U);
    EXPECT_EQ(configuration.randomSeed(), 42U);
}