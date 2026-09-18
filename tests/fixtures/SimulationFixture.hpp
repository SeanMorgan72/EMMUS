#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "TestFixture.hpp"

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/simulation/Simulation.hpp"

namespace emmus::test {

class SimulationFixture : public TestFixture {
protected:
    static constexpr std::size_t kDefaultFrameCount = 4U;
    static constexpr std::uint64_t kDefaultSeed = 12345U;

    static emmus::infrastructure::configuration::SimulationConfiguration
    makeConfiguration(
        std::size_t processCount,
        std::size_t pageCountPerProcess,
        std::size_t memoryAccessCount,
        std::size_t frameCount,
        emmus::algorithms::replacement::PageReplacementPolicyType replacementPolicy,
        emmus::infrastructure::configuration::WorkloadType workloadType,
        std::uint64_t randomSeed = kDefaultSeed)
    {
        return emmus::infrastructure::configuration::SimulationConfiguration(
            emmus::memory::access::PageSize{4096},
            emmus::memory::access::FrameCount{frameCount},
            processCount,
            pageCountPerProcess,
            replacementPolicy,
            workloadType,
            memoryAccessCount,
            randomSeed,
            0.65,
            0.35,
            std::min<std::size_t>(pageCountPerProcess, 3U));
    }

    static void expectSuccessfulRun(
        const emmus::simulation::SimulationResult& result,
        std::size_t expectedAccessCount)
    {
        EXPECT_TRUE(result.succeeded());
        EXPECT_EQ(result.status(), emmus::simulation::SimulationStatus::Completed);
        EXPECT_EQ(result.configuration().memoryAccessCount(), expectedAccessCount);
        EXPECT_EQ(result.executionResult().size(), expectedAccessCount);
        EXPECT_EQ(
            result.executionResult().statistics().memoryAccessCount(),
            expectedAccessCount);
        EXPECT_EQ(
            result.pageFaultStatistics().totalAccessCount(),
            expectedAccessCount);
        EXPECT_EQ(
            result.pageFaultStatistics().totalPageFaultCount(),
            result.executionResult().statistics().pageFaultCount());
        EXPECT_EQ(
            result.pageReplacementStatistics().replacementCount(),
            result.executionResult().statistics().pageReplacementCount());
        EXPECT_EQ(
            result.pageReplacementStatistics().dirtyEvictionCount(),
            result.executionResult().statistics().dirtyEvictionCount());
    }
};

} // namespace emmus::test