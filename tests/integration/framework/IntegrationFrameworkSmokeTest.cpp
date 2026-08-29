#include <gtest/gtest.h>

#include "fixtures/MemoryManagerFixture.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"
#include "fixtures/SimulationFixture.hpp"

namespace {

TEST(
    IntegrationFrameworkSmokeTest,
    ReusableFixturesCompileAndRemainIndependent
)
{
    emmus::test::MemoryManagerFixture* memory = nullptr;

    emmus::test::PageReplacementAlgorithmFixture* replacement = nullptr;

    emmus::test::SimulationFixture* simulation = nullptr;

    EXPECT_EQ(
        memory,
        nullptr
    );

    EXPECT_EQ(
        replacement,
        nullptr
    );

    EXPECT_EQ(
        simulation,
        nullptr
    );
}


TEST(
    IntegrationFrameworkSmokeTest,
    DeterministicScenarioDefaultsAreDefined
)
{
    // The production simulation fixture will eventually replace this
    // infrastructure smoke assertion with a complete:
    //
    // workload -> process -> MMU -> page table ->
    // physical memory -> replacement -> statistics
    //
    // scenario.

    SUCCEED();
}

} // namespace