#include <gtest/gtest.h>

#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/simulation/Simulation.hpp"

namespace {

using emmus::algorithms::replacement::
    PageReplacementPolicyType;
using emmus::infrastructure::configuration::
    SimulationConfiguration;
using emmus::infrastructure::configuration::
    WorkloadType;

TEST(CompleteSimulationExecutionTest,
     ExecutesConfiguredWorkloadWithoutGui) {

    constexpr std::size_t accessCount = 12;

    SimulationConfiguration configuration(
        emmus::memory::access::PageSize{4096},
        emmus::memory::access::FrameCount{3},
        1,
        5,
        PageReplacementPolicyType::FIFO,
        WorkloadType::Sequential,
        accessCount,
        42);

    emmus::simulation::Simulation simulation(configuration);

    const auto result = simulation.run();

    ASSERT_TRUE(result.succeeded());

    EXPECT_EQ(
        result.executionResult().size(),
        accessCount);
}

} // namespace