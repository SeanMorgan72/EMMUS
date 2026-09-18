#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/simulation/Simulation.hpp"
#include "fixtures/SimulationFixture.hpp"

namespace emmus::system::test {

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::infrastructure::configuration::MixedWorkloadSegmentConfiguration;
using emmus::infrastructure::configuration::SimulationConfiguration;
using emmus::infrastructure::configuration::WorkloadType;
using emmus::simulation::Simulation;
using emmus::simulation::SimulationResult;
using emmus::simulation::SimulationStatus;

class EndToEndSimulationSystemTest : public emmus::test::SimulationFixture {
protected:
    static SimulationConfiguration makeMixedConfiguration(
        std::size_t processCount,
        std::size_t pageCountPerProcess,
        std::size_t memoryAccessCount,
        std::size_t frameCount,
        PageReplacementPolicyType replacementPolicy,
        std::uint64_t randomSeed)
    {
        auto configuration = SimulationConfiguration(
            emmus::memory::access::PageSize{4096},
            emmus::memory::access::FrameCount{frameCount},
            processCount,
            pageCountPerProcess,
            replacementPolicy,
            WorkloadType::Mixed,
            memoryAccessCount,
            randomSeed,
            0.8,
            0.5,
            std::min<std::size_t>(pageCountPerProcess, 4U));

        std::vector<MixedWorkloadSegmentConfiguration> segments = {
            {WorkloadType::Sequential, 8U},
            {WorkloadType::Random, 10U},
            {WorkloadType::Locality, 12U},
            {WorkloadType::Random, 6U}};

        std::size_t total = 0U;
        for (const auto& segment : segments) {
            total += segment.accessCount;
        }

        if (total != memoryAccessCount) {
            std::vector<MixedWorkloadSegmentConfiguration> adjusted;
            adjusted.reserve(segments.size());

            std::size_t remaining = memoryAccessCount;
            for (std::size_t index = 0; index < segments.size(); ++index) {
                const auto& segment = segments[index];
                const std::size_t chosen =
                    (index + 1U == segments.size())
                        ? remaining
                        : std::min(segment.accessCount, remaining);

                adjusted.push_back({segment.workloadType, chosen});
                remaining -= chosen;
                if (remaining == 0U) {
                    break;
                }
            }

            configuration.setMixedWorkloadSegments(std::move(adjusted));
            return configuration;
        }

        configuration.setMixedWorkloadSegments(std::move(segments));
        return configuration;
    }
};

TEST_F(EndToEndSimulationSystemTest, SequentialWorkflowCompletesEndToEnd) {
    const auto configuration = makeConfiguration(
        1U,
        6U,
        24U,
        3U,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Sequential,
        2024U);

    Simulation simulation(configuration);
    const auto result = simulation.run();

    expectSuccessfulRun(result, configuration.memoryAccessCount());

    EXPECT_EQ(
        result.pageFaultStatistics().faultRate(),
        result.executionResult().statistics().pageFaultRate());

    EXPECT_GE(
        result.executionResult().statistics().pageFaultCount(),
        1U);
}

TEST_F(EndToEndSimulationSystemTest, MultiProcessRandomWorkflowCoordinatesSubsystems) {
    const auto configuration = makeConfiguration(
        3U,
        8U,
        90U,
        4U,
        PageReplacementPolicyType::LRU,
        WorkloadType::Random,
        314159U);

    Simulation simulation(configuration);
    const auto result = simulation.run();

    expectSuccessfulRun(result, configuration.memoryAccessCount());

    std::set<std::uint64_t> uniqueProcessIds;
    for (const auto& record : result.executionResult().records()) {
        uniqueProcessIds.insert(record.access.processId().value());
    }

    EXPECT_EQ(uniqueProcessIds.size(), configuration.processCount());
    EXPECT_GT(result.executionResult().statistics().pageFaultCount(), 0U);
    EXPECT_GT(result.pageFaultStatistics().totalPageFaultCount(), 0U);
    EXPECT_GE(result.pageReplacementStatistics().replacementCount(), 0U);
}

TEST_F(EndToEndSimulationSystemTest, LocalityScenarioProducesStableStatistics) {
    const auto configuration = makeConfiguration(
        2U,
        10U,
        80U,
        5U,
        PageReplacementPolicyType::CLOCK,
        WorkloadType::Locality,
        271828U);

    Simulation simulation(configuration);
    const auto result = simulation.run();

    expectSuccessfulRun(result, configuration.memoryAccessCount());

    EXPECT_GT(result.executionResult().statistics().pageFaultCount(), 0U);
    EXPECT_EQ(
        result.pageFaultStatistics().processCount(),
        configuration.processCount());
    EXPECT_GE(result.pageFaultStatistics().faultRate(), 0.0);
    EXPECT_LE(result.pageFaultStatistics().faultRate(), 1.0);
}

TEST_F(EndToEndSimulationSystemTest, MixedWorkloadRespectsConfiguredSegmentsAndTotals) {
    const auto configuration = makeMixedConfiguration(
        2U,
        7U,
        36U,
        4U,
        PageReplacementPolicyType::OPTIMAL,
        987654321U);

    Simulation simulation(configuration);
    const auto result = simulation.run();

    expectSuccessfulRun(result, configuration.memoryAccessCount());

    const auto& segments = configuration.mixedWorkloadSegments();
    std::size_t totalSegmentAccesses = 0;
    for (const auto& segment : segments) {
        totalSegmentAccesses += segment.accessCount;
    }

    EXPECT_EQ(totalSegmentAccesses, configuration.memoryAccessCount());
    EXPECT_EQ(result.executionResult().records().size(), configuration.memoryAccessCount());

    std::size_t uniqueProcessCount = 0;
    std::set<std::uint64_t> observedProcesses;
    for (const auto& record : result.executionResult().records()) {
        observedProcesses.insert(record.access.processId().value());
    }
    uniqueProcessCount = observedProcesses.size();

    EXPECT_GE(uniqueProcessCount, 1U);
    EXPECT_LE(uniqueProcessCount, configuration.processCount());
}

TEST_F(EndToEndSimulationSystemTest, InvalidConfigurationFailsBeforeExecution) {
    auto invalidConfiguration = makeConfiguration(
        0U,
        8U,
        30U,
        4U,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Random,
        42U);

    EXPECT_THROW(
        {
            Simulation simulation(invalidConfiguration);
            (void)simulation;
        },
        std::invalid_argument);
}

TEST_F(EndToEndSimulationSystemTest, SameSeedProducesDeterministicSystemState) {
    const auto configuration = makeConfiguration(
        2U,
        6U,
        64U,
        3U,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Random,
        424242U);

    Simulation firstSimulation(configuration);
    Simulation secondSimulation(configuration);

    const auto firstResult = firstSimulation.run();
    const auto secondResult = secondSimulation.run();

    ASSERT_TRUE(firstResult.succeeded());
    ASSERT_TRUE(secondResult.succeeded());

    EXPECT_EQ(
        firstResult.executionResult().records(),
        secondResult.executionResult().records());

    EXPECT_EQ(
        firstResult.pageFaultStatistics(),
        secondResult.pageFaultStatistics());

    EXPECT_EQ(
        firstResult.pageReplacementStatistics().replacementCount(),
        secondResult.pageReplacementStatistics().replacementCount());

    EXPECT_EQ(
        firstResult.executionResult().statistics().pageFaultCount(),
        secondResult.executionResult().statistics().pageFaultCount());
}

TEST_F(EndToEndSimulationSystemTest, ReplacementPoliciesRemainCompatibleAcrossSystemWorkflow) {
    constexpr PageReplacementPolicyType policies[] = {
        PageReplacementPolicyType::FIFO,
        PageReplacementPolicyType::LRU,
        PageReplacementPolicyType::CLOCK,
        PageReplacementPolicyType::OPTIMAL};

    for (const auto policy : policies) {
        const auto configuration = makeConfiguration(
            2U,
            7U,
            120U,
            4U,
            policy,
            WorkloadType::Random,
            99U);

        EXPECT_NO_THROW({
            Simulation simulation(configuration);
            const auto result = simulation.run();
            EXPECT_TRUE(result.succeeded());
            EXPECT_EQ(result.executionResult().size(), configuration.memoryAccessCount());
        });
    }
}

} // namespace emmus::system::test
