#include <cstddef>
#include <cstdint>
#include <set>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/simulation/Simulation.hpp"

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::SimulationConfiguration;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::memory::access::FrameCount;
using emmus::memory::access::PageSize;
using emmus::simulation::Simulation;

namespace {

SimulationConfiguration makeConfiguration(
    std::size_t processCount,
    std::size_t pageCountPerProcess,
    std::size_t memoryAccessCount,
    std::size_t frameCount,
    PageReplacementPolicyType replacementPolicy,
    WorkloadType workloadType = WorkloadType::Random,
    std::uint64_t randomSeed = 12345)
{
    return SimulationConfiguration(
        PageSize{4096},
        FrameCount{frameCount},
        processCount,
        pageCountPerProcess,
        replacementPolicy,
        workloadType,
        memoryAccessCount,
        randomSeed);
}

} // namespace

TEST(SimulationTest, ExecutesConfiguredNumberOfMemoryAccesses)
{
    const auto configuration = makeConfiguration(
        1,
        6,
        20,
        3,
        PageReplacementPolicyType::FIFO);

    Simulation simulation(configuration);

    const auto result = simulation.run();

    EXPECT_EQ(
        result.executionResult().size(),
        configuration.memoryAccessCount());

    EXPECT_EQ(
        result.executionResult().statistics().memoryAccessCount(),
        configuration.memoryAccessCount());
}

TEST(SimulationTest, CompleteRunProducesPageFaults)
{
    const auto configuration = makeConfiguration(
        1,
        4,
        8,
        2,
        PageReplacementPolicyType::FIFO);

    Simulation simulation(configuration);

    const auto result = simulation.run();

    EXPECT_GT(
        result.executionResult().statistics().pageFaultCount(),
        0U);
}

TEST(SimulationTest, SimulationIsRepeatableWithSameSeed)
{
    const auto configuration = makeConfiguration(
        2,
        6,
        50,
        3,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Random,
        12345);

    Simulation simulation(configuration);

    const auto first = simulation.run();
    const auto second = simulation.run();

    ASSERT_EQ(
        first.executionResult().size(),
        second.executionResult().size());

    ASSERT_EQ(
        first.executionResult().records().size(),
        second.executionResult().records().size());

    for (std::size_t i = 0;
         i < first.executionResult().records().size();
         ++i)
    {
        const auto& firstRecord =
            first.executionResult().records()[i];

        const auto& secondRecord =
            second.executionResult().records()[i];

        EXPECT_EQ(
            firstRecord.access,
            secondRecord.access);

        EXPECT_EQ(
            firstRecord.result,
            secondRecord.result);
    }

    EXPECT_EQ(
        first.executionResult().statistics().memoryAccessCount(),
        second.executionResult().statistics().memoryAccessCount());

    EXPECT_EQ(
        first.executionResult().statistics().pageFaultCount(),
        second.executionResult().statistics().pageFaultCount());

    EXPECT_EQ(
        first.executionResult().statistics().pageReplacementCount(),
        second.executionResult().statistics().pageReplacementCount());

    EXPECT_EQ(
        first.executionResult().statistics().dirtyEvictionCount(),
        second.executionResult().statistics().dirtyEvictionCount());
}

TEST(SimulationTest, SupportsAllRegisteredReplacementPolicies)
{
    constexpr PageReplacementPolicyType policies[] = {
        PageReplacementPolicyType::FIFO,
        PageReplacementPolicyType::LRU,
        PageReplacementPolicyType::CLOCK,
        PageReplacementPolicyType::OPTIMAL
    };

    for (const auto policy : policies)
    {
        const auto configuration = makeConfiguration(
            1,
            6,
            20,
            3,
            policy);

        EXPECT_NO_THROW({
            Simulation simulation(configuration);
            const auto result = simulation.run();

            EXPECT_EQ(
                result.executionResult().size(),
                configuration.memoryAccessCount());
        });
    }
}

TEST(SimulationTest, SupportsMultipleProcesses)
{
    const auto configuration = makeConfiguration(
        3,
        4,
        30,
        3,
        PageReplacementPolicyType::LRU);

    Simulation simulation(configuration);

    const auto result = simulation.run();

    EXPECT_EQ(
        result.executionResult().size(),
        configuration.memoryAccessCount());

    std::set<std::uint64_t> processIds;

    for (const auto& record : result.executionResult().records())
    {
        processIds.insert(
            record.access.processId().value());
    }

    EXPECT_EQ(processIds.size(), 3U);
}

TEST(SimulationTest, SupportsSequentialWorkload)
{
    const auto configuration = makeConfiguration(
        1,
        6,
        20,
        3,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Sequential);

    Simulation simulation(configuration);

    const auto result = simulation.run();

    EXPECT_EQ(
        result.executionResult().size(),
        configuration.memoryAccessCount());

    EXPECT_GT(
        result.executionResult().statistics().pageFaultCount(),
        0U);
}