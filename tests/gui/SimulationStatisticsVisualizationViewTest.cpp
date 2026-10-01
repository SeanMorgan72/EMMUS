#include <array>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include "SimulationStatisticsVisualizationView.hpp"
#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/SimulationConfiguration.hpp"
#include "emmus/memory/access/MemoryAccessTypes.hpp"
#include "emmus/simulation/Simulation.hpp"

namespace
{

using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::gui::SimulationStatisticsVisualizationView;
using emmus::infrastructure::configuration::SimulationConfiguration;
using emmus::infrastructure::configuration::WorkloadType;

SimulationConfiguration makeConfiguration(
    PageReplacementPolicyType policy,
    std::size_t accessCount = 24U)
{
    return SimulationConfiguration(
        emmus::memory::access::PageSize{4096},
        emmus::memory::access::FrameCount{3},
        1U,
        6U,
        policy,
        WorkloadType::Random,
        accessCount,
        7261U);
}

TEST(SimulationStatisticsVisualizationViewTest, StartsEmptyAndResetClearsAllState)
{
    SimulationStatisticsVisualizationView view;

    EXPECT_FALSE(view.hasSnapshot());
    EXPECT_FALSE(view.comparison().has_value());
    EXPECT_NE(view.render().find("[no statistics available]"), std::string::npos);
    EXPECT_EQ(view.renderCompact(), "{no statistics}");

    emmus::simulation::SimulationProgress progress;
    progress.totalAccessCount = 1U;
    view.update(progress);
    view.setComparison(view.snapshot());
    view.reset();

    EXPECT_FALSE(view.hasSnapshot());
    EXPECT_FALSE(view.comparison().has_value());
}

TEST(SimulationStatisticsVisualizationViewTest, RendersLiveProgressAndZeroAccessEdgeCase)
{
    SimulationStatisticsVisualizationView view;
    emmus::simulation::SimulationProgress progress;
    progress.totalAccessCount = 0U;

    view.update(progress);

    const auto report = view.render();
    EXPECT_NE(report.find("Status: RUNNING"), std::string::npos);
    EXPECT_NE(report.find("Memory accesses: 0/0"), std::string::npos);
    EXPECT_NE(report.find("0.0% fault rate"), std::string::npos);
    EXPECT_NE(report.find("Memory utilization: 0.0%"), std::string::npos);
}

TEST(SimulationStatisticsVisualizationViewTest, MatchesCompletedSimulationData)
{
    emmus::simulation::Simulation simulation(
        makeConfiguration(PageReplacementPolicyType::FIFO));
    const auto result = simulation.run();

    SimulationStatisticsVisualizationView view;
    view.update(result, simulation.physicalMemoryManager().snapshotUtilization());

    const auto& snapshot = view.snapshot();
    const auto& execution = result.executionResult().statistics();
    EXPECT_EQ(snapshot.status, SimulationStatisticsVisualizationView::Status::Completed);
    EXPECT_EQ(snapshot.memoryAccessCount, execution.memoryAccessCount());
    EXPECT_EQ(snapshot.successfulAccessCount, execution.successfulAccessCount());
    EXPECT_EQ(snapshot.failedAccessCount, execution.failedAccessCount());
    EXPECT_EQ(snapshot.pageFaultCount, result.pageFaultStatistics().totalPageFaultCount());
    EXPECT_EQ(snapshot.pageReplacementCount, result.pageReplacementStatistics().replacementCount());
    EXPECT_EQ(snapshot.dirtyEvictionCount, result.pageReplacementStatistics().dirtyEvictionCount());
    EXPECT_EQ(snapshot.executionTime, result.executionTime());
    EXPECT_EQ(snapshot.replacementExecutionTime,
              result.pageReplacementStatistics().totalExecutionTime());

    const auto report = view.render();
    EXPECT_NE(report.find("Status: COMPLETED"), std::string::npos);
    EXPECT_NE(report.find("Replacement policy: FIFO"), std::string::npos);
    EXPECT_NE(report.find("Page faults: " + std::to_string(snapshot.pageFaultCount)),
              std::string::npos);
    EXPECT_NE(report.find("Dirty-page evictions: " +
                          std::to_string(snapshot.dirtyEvictionCount)),
              std::string::npos);
    EXPECT_NE(report.find("Memory utilization:"), std::string::npos);
    EXPECT_NE(report.find("Execution time:"), std::string::npos);
}

TEST(SimulationStatisticsVisualizationViewTest, UpdatesFromEachLiveSimulationSnapshot)
{
    emmus::simulation::Simulation simulation(
        makeConfiguration(PageReplacementPolicyType::CLOCK, 12U));
    SimulationStatisticsVisualizationView view;
    std::size_t updateCount = 0U;

    const auto result = simulation.run(
        [&view, &updateCount](const emmus::simulation::SimulationProgress& progress) {
            view.update(progress);
            ++updateCount;
        });

    EXPECT_EQ(updateCount, result.executionResult().size());
    ASSERT_TRUE(view.hasSnapshot());
    EXPECT_EQ(
        view.snapshot().memoryAccessCount,
        result.executionResult().statistics().memoryAccessCount());
    EXPECT_EQ(
        view.snapshot().pageFaultCount,
        result.pageFaultStatistics().totalPageFaultCount());
    EXPECT_EQ(
        view.snapshot().memory.totalFrames,
        simulation.physicalMemoryManager().snapshotUtilization().totalFrames);
    EXPECT_EQ(view.snapshot().status, SimulationStatisticsVisualizationView::Status::Running);

    view.update(result, simulation.physicalMemoryManager().snapshotUtilization());
    EXPECT_EQ(view.snapshot().status, SimulationStatisticsVisualizationView::Status::Completed);
}

TEST(SimulationStatisticsVisualizationViewTest, SupportsEveryReplacementPolicy)
{
    constexpr std::array policies{
        PageReplacementPolicyType::FIFO,
        PageReplacementPolicyType::LRU,
        PageReplacementPolicyType::CLOCK,
        PageReplacementPolicyType::OPTIMAL};
    constexpr std::array names{"FIFO", "LRU", "CLOCK", "OPTIMAL"};

    for (std::size_t index = 0U; index < policies.size(); ++index)
    {
        emmus::simulation::Simulation simulation(makeConfiguration(policies[index]));
        const auto result = simulation.run();
        SimulationStatisticsVisualizationView view;
        view.update(result, simulation.physicalMemoryManager().snapshotUtilization());

        EXPECT_NE(
            view.render().find("Replacement policy: " + std::string(names[index])),
            std::string::npos);
        EXPECT_EQ(
            view.snapshot().pageReplacementCount,
            result.executionResult().statistics().pageReplacementCount());
    }
}

TEST(SimulationStatisticsVisualizationViewTest, ComparesRunsAndCanClearBaseline)
{
    emmus::simulation::Simulation baselineSimulation(
        makeConfiguration(PageReplacementPolicyType::FIFO));
    const auto baselineResult = baselineSimulation.run();

    emmus::simulation::Simulation currentSimulation(
        makeConfiguration(PageReplacementPolicyType::LRU));
    const auto currentResult = currentSimulation.run();

    SimulationStatisticsVisualizationView view;
    SimulationStatisticsVisualizationView baselineView;
    baselineView.update(
        baselineResult,
        baselineSimulation.physicalMemoryManager().snapshotUtilization());
    view.update(
        currentResult,
        currentSimulation.physicalMemoryManager().snapshotUtilization());
    view.setComparison(baselineView.snapshot());

    auto report = view.render();
    EXPECT_NE(report.find("Comparison: FIFO -> LRU"), std::string::npos);
    EXPECT_NE(report.find("Page replacements: "), std::string::npos);
    EXPECT_NE(report.find("Execution time: "), std::string::npos);

    view.clearComparison();
    report = view.render();
    EXPECT_EQ(report.find("Comparison:"), std::string::npos);
    EXPECT_FALSE(view.comparison().has_value());
}

} // namespace