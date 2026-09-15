#include "emmus/simulation/Simulation.hpp"

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"

namespace emmus::simulation {
namespace {

using algorithms::replacement::PageReplacementPolicyType;
using infrastructure::configuration::SimulationConfiguration;
using infrastructure::configuration::WorkloadType;
using memory::access::FrameCount;
using memory::access::PageSize;

SimulationConfiguration makeLocalityConfiguration(
    std::size_t processCount = 1U,
    std::size_t pageCount = 16U,
    std::size_t accessCount = 100U,
    std::size_t frameCount = 4U,
    double temporalStrength = 0.75,
    double spatialStrength = 0.75,
    std::size_t workingSetSize = 4U,
    std::uint64_t seed = 12345U) {

    return SimulationConfiguration(
        PageSize{4096U},
        FrameCount{frameCount},
        processCount,
        pageCount,
        PageReplacementPolicyType::LRU,
        WorkloadType::Locality,
        accessCount,
        seed,
        temporalStrength,
        spatialStrength,
        workingSetSize);
}

} // namespace

TEST(
    LocalityWorkloadIntegrationTest,
    LocalityWorkloadRunsThroughSimulation) {

    Simulation simulation(
        makeLocalityConfiguration());

    const auto result =
        simulation.run();

    EXPECT_EQ(
        result.status(),
        SimulationStatus::Completed);

    EXPECT_EQ(
        result.executionResult()
            .statistics()
            .memoryAccessCount(),
        100U);
}

TEST(
    LocalityWorkloadIntegrationTest,
    SameConfigurationAndSeedProducesSameStatistics) {

    Simulation first(
        makeLocalityConfiguration(
            1U,
            16U,
            200U,
            4U,
            0.8,
            0.8,
            4U,
            98765U));

    Simulation second(
        makeLocalityConfiguration(
            1U,
            16U,
            200U,
            4U,
            0.8,
            0.8,
            4U,
            98765U));

    const auto firstResult = first.run();
    const auto secondResult = second.run();

    const auto& firstStatistics =
        firstResult.executionResult().statistics();

    const auto& secondStatistics =
        secondResult.executionResult().statistics();

    EXPECT_EQ(
        firstStatistics.memoryAccessCount(),
        secondStatistics.memoryAccessCount());

    EXPECT_EQ(
        firstStatistics.successfulAccessCount(),
        secondStatistics.successfulAccessCount());

    EXPECT_EQ(
        firstStatistics.failedAccessCount(),
        secondStatistics.failedAccessCount());

    EXPECT_EQ(
        firstStatistics.pageFaultCount(),
        secondStatistics.pageFaultCount());

    EXPECT_EQ(
        firstStatistics.pageReplacementCount(),
        secondStatistics.pageReplacementCount());

    EXPECT_EQ(
        firstStatistics.dirtyEvictionCount(),
        secondStatistics.dirtyEvictionCount());
}

TEST(
    LocalityWorkloadIntegrationTest,
    MultipleProcessesProduceConfiguredAccessCount) {

    constexpr std::size_t processCount = 3U;
    constexpr std::size_t accessCount = 101U;

    Simulation simulation(
        makeLocalityConfiguration(
            processCount,
            16U,
            accessCount,
            6U,
            0.8,
            0.8,
            4U,
            123U));

    const auto result =
        simulation.run();

    EXPECT_EQ(
        result.executionResult()
            .statistics()
            .memoryAccessCount(),
        accessCount);
}

TEST(
    LocalityWorkloadIntegrationTest,
    SmallerWorkingSetProducesSuccessfulSimulation) {

    Simulation simulation(
        makeLocalityConfiguration(
            1U,
            64U,
            500U,
            8U,
            1.0,
            1.0,
            2U,
            42U));

    const auto result =
        simulation.run();

    EXPECT_EQ(
        result.status(),
        SimulationStatus::Completed);

    EXPECT_EQ(
        result.executionResult()
            .statistics()
            .memoryAccessCount(),
        500U);
}

} // namespace emmus::simulation