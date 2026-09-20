#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"
#include "emmus/statistics/PageFaultComparison.hpp"
#include "emmus/simulation/SimulationComparison.hpp"

namespace emmus::test
{
namespace
{

using PolicyType = emmus::algorithms::replacement::PageReplacementPolicyType;
using PageFaultComparison = emmus::statistics::PageFaultComparison;

TEST(PageFaultComparisonTest, RecordsEachAlgorithmResultAndRanksFaults)
{
    PageFaultComparison comparison;

    emmus::statistics::PageFaultStatistics fifoStatistics;
    fifoStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    fifoStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    fifoStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    fifoStatistics.recordPageFault(emmus::memory::identifiers::ProcessId{1U});
    fifoStatistics.recordPageFault(emmus::memory::identifiers::ProcessId{1U});

    emmus::statistics::PageFaultStatistics lruStatistics;
    lruStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    lruStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    lruStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    lruStatistics.recordPageFault(emmus::memory::identifiers::ProcessId{1U});

    emmus::statistics::PageFaultStatistics optimalStatistics;
    optimalStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    optimalStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    optimalStatistics.recordAccess(emmus::memory::identifiers::ProcessId{1U});

    comparison.recordResult(PolicyType::FIFO, fifoStatistics);
    comparison.recordResult(PolicyType::LRU, lruStatistics);
    comparison.recordResult(PolicyType::OPTIMAL, optimalStatistics);

    ASSERT_EQ(comparison.size(), 3U);
    ASSERT_EQ(comparison.results().size(), 3U);

    const auto& ranked = comparison.results();
    EXPECT_EQ(ranked.front().policy, PolicyType::OPTIMAL);
    EXPECT_EQ(ranked[1].policy, PolicyType::LRU);
    EXPECT_EQ(ranked.back().policy, PolicyType::FIFO);

    EXPECT_EQ(ranked.front().pageFaultCount, 0U);
    EXPECT_EQ(ranked[1].pageFaultCount, 1U);
    EXPECT_EQ(ranked.back().pageFaultCount, 2U);
}

TEST(PageFaultComparisonTest, GeneratesReadableAlgorithmSummary)
{
    PageFaultComparison comparison;

    emmus::statistics::PageFaultStatistics stats;
    stats.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    stats.recordAccess(emmus::memory::identifiers::ProcessId{1U});
    stats.recordPageFault(emmus::memory::identifiers::ProcessId{1U});

    comparison.recordResult(PolicyType::FIFO, stats);

    const auto summary = comparison.summary();
    EXPECT_NE(summary.find("FIFO"), std::string::npos);
    EXPECT_NE(summary.find("faults"), std::string::npos);
    EXPECT_NE(summary.find("fault rate"), std::string::npos);
}

TEST(PageFaultComparisonTest, SimulationComparisonRunsAllEquivalentPolicies)
{
    const emmus::infrastructure::configuration::BenchmarkConfiguration benchmark(
        emmus::memory::access::PageSize{4096},
        emmus::memory::access::FrameCount{4},
        2U,
        8U,
        60U,
        emmus::infrastructure::configuration::WorkloadType::Random,
        2024U,
        PolicyType::FIFO);

    const emmus::simulation::SimulationComparison comparison(benchmark);
    const auto results = comparison.run();

    EXPECT_EQ(results.size(), 4U);
    EXPECT_TRUE(results.hasPolicy(PolicyType::FIFO));
    EXPECT_TRUE(results.hasPolicy(PolicyType::LRU));
    EXPECT_TRUE(results.hasPolicy(PolicyType::CLOCK));
    EXPECT_TRUE(results.hasPolicy(PolicyType::OPTIMAL));

    for (const auto& result : results.results())
    {
        EXPECT_GE(result.pageFaultCount, 0U);
        EXPECT_GE(result.faultRate, 0.0);
        EXPECT_LE(result.faultRate, 1.0);
    }
}

} // namespace

} // namespace emmus::test
