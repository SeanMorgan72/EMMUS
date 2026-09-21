#include <chrono>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"
#include "emmus/infrastructure/configuration/BenchmarkConfiguration.hpp"
#include "emmus/simulation/SimulationComparison.hpp"
#include "emmus/statistics/ExecutionTimeComparison.hpp"

namespace emmus::test
{
namespace
{

using PolicyType = emmus::algorithms::replacement::PageReplacementPolicyType;
using ExecutionTimeComparison = emmus::statistics::ExecutionTimeComparison;

TEST(ExecutionTimeComparisonTest, RecordsEachAlgorithmTimingResultAndRanksFastest)
{
    ExecutionTimeComparison comparison;

    emmus::statistics::PageReplacementStatistics fifoStatistics;
    fifoStatistics.recordReplacement();
    fifoStatistics.recordExecutionTime(std::chrono::microseconds{500});

    emmus::statistics::PageReplacementStatistics lruStatistics;
    lruStatistics.recordReplacement();
    lruStatistics.recordExecutionTime(std::chrono::microseconds{200});

    emmus::statistics::PageReplacementStatistics optimalStatistics;
    optimalStatistics.recordReplacement();
    optimalStatistics.recordExecutionTime(std::chrono::microseconds{100});

    comparison.recordResult(PolicyType::FIFO, fifoStatistics, std::chrono::milliseconds{5});
    comparison.recordResult(PolicyType::LRU, lruStatistics, std::chrono::milliseconds{4});
    comparison.recordResult(PolicyType::OPTIMAL, optimalStatistics, std::chrono::milliseconds{3});

    ASSERT_EQ(comparison.size(), 3U);
    ASSERT_EQ(comparison.results().size(), 3U);

    const auto& ranked = comparison.results();
    EXPECT_EQ(ranked.front().policy, PolicyType::OPTIMAL);
    EXPECT_EQ(ranked[1].policy, PolicyType::LRU);
    EXPECT_EQ(ranked.back().policy, PolicyType::FIFO);

    EXPECT_DOUBLE_EQ(ranked.front().averageReplacementTimeMilliseconds(), 0.1);
    EXPECT_DOUBLE_EQ(ranked[1].averageReplacementTimeMilliseconds(), 0.2);
    EXPECT_DOUBLE_EQ(ranked.back().averageReplacementTimeMilliseconds(), 0.5);
}

TEST(ExecutionTimeComparisonTest, GeneratesReadableAlgorithmSummary)
{
    ExecutionTimeComparison comparison;

    emmus::statistics::PageReplacementStatistics stats;
    stats.recordReplacement();
    stats.recordExecutionTime(std::chrono::microseconds{250});

    comparison.recordResult(PolicyType::FIFO, stats, std::chrono::milliseconds{10});

    const auto summary = comparison.summary();
    EXPECT_NE(summary.find("FIFO"), std::string::npos);
    EXPECT_NE(summary.find("average"), std::string::npos);
    EXPECT_NE(summary.find("Execution time"), std::string::npos);
}

TEST(ExecutionTimeComparisonTest, SimulationComparisonRunsExecutionTimeComparisonForAllPolicies)
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
    const auto results = comparison.runExecutionTimeComparison();

    EXPECT_EQ(results.size(), 4U);
    EXPECT_TRUE(results.hasPolicy(PolicyType::FIFO));
    EXPECT_TRUE(results.hasPolicy(PolicyType::LRU));
    EXPECT_TRUE(results.hasPolicy(PolicyType::CLOCK));
    EXPECT_TRUE(results.hasPolicy(PolicyType::OPTIMAL));

    for (const auto& result : results.results())
    {
        EXPECT_GE(result.totalSimulationTime.count(), 0);
        EXPECT_GE(result.totalReplacementTime().count(), 0);
        EXPECT_GE(result.averageReplacementTimeMilliseconds(), 0.0);
        EXPECT_GE(result.simulationTimeMilliseconds(), 0.0);
    }
}

} // namespace

} // namespace emmus::test
