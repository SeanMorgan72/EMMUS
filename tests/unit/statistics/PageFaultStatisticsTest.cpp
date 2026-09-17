#include "emmus/statistics/PageFaultStatistics.hpp"

#include <gtest/gtest.h>

namespace emmus::test
{

namespace
{

using ProcessId = emmus::memory::identifiers::ProcessId;
using Statistics = emmus::statistics::PageFaultStatistics;

TEST(PageFaultStatisticsTest, InitialStatisticsAreZero)
{
    const Statistics statistics;

    EXPECT_EQ(statistics.totalAccessCount(), 0U);
    EXPECT_EQ(statistics.totalPageFaultCount(), 0U);
    EXPECT_EQ(statistics.processCount(), 0U);
    EXPECT_DOUBLE_EQ(statistics.faultRate(), 0.0);
}

TEST(PageFaultStatisticsTest, RecordsPerProcessTotalsAndRates)
{
    Statistics statistics;

    const ProcessId processOne{1U};
    const ProcessId processTwo{2U};

    statistics.recordAccess(processOne);
    statistics.recordAccess(processOne);
    statistics.recordPageFault(processOne);

    statistics.recordAccess(processTwo);
    statistics.recordAccess(processTwo);
    statistics.recordPageFault(processTwo);

    EXPECT_EQ(statistics.totalAccessCount(), 4U);
    EXPECT_EQ(statistics.totalPageFaultCount(), 2U);
    EXPECT_DOUBLE_EQ(statistics.faultRate(), 0.5);

    EXPECT_EQ(statistics.accessCount(processOne), 2U);
    EXPECT_EQ(statistics.pageFaultCount(processOne), 1U);
    EXPECT_DOUBLE_EQ(statistics.faultRate(processOne), 0.5);

    EXPECT_EQ(statistics.accessCount(processTwo), 2U);
    EXPECT_EQ(statistics.pageFaultCount(processTwo), 1U);
    EXPECT_DOUBLE_EQ(statistics.faultRate(processTwo), 0.5);
}

TEST(PageFaultStatisticsTest, ZeroFaultScenarioKeepsRateAtZero)
{
    Statistics statistics;

    const ProcessId processId{42U};

    statistics.recordAccess(processId);
    statistics.recordAccess(processId);
    statistics.recordAccess(processId);

    EXPECT_EQ(statistics.totalAccessCount(), 3U);
    EXPECT_EQ(statistics.totalPageFaultCount(), 0U);
    EXPECT_DOUBLE_EQ(statistics.faultRate(), 0.0);
    EXPECT_DOUBLE_EQ(statistics.faultRate(processId), 0.0);
}

TEST(PageFaultStatisticsTest, ResetClearsAllCounters)
{
    Statistics statistics;

    const ProcessId processId{9U};

    statistics.recordAccess(processId);
    statistics.recordPageFault(processId);
    statistics.reset();

    EXPECT_EQ(statistics.totalAccessCount(), 0U);
    EXPECT_EQ(statistics.totalPageFaultCount(), 0U);
    EXPECT_EQ(statistics.accessCount(processId), 0U);
    EXPECT_EQ(statistics.pageFaultCount(processId), 0U);
    EXPECT_DOUBLE_EQ(statistics.faultRate(), 0.0);
    EXPECT_EQ(statistics.processCount(), 0U);
}

TEST(PageFaultStatisticsTest, EqualityComparisonMatchesState)
{
    Statistics left;
    Statistics right;

    const ProcessId processId{7U};

    left.recordAccess(processId);
    left.recordPageFault(processId);

    right.recordAccess(processId);
    right.recordPageFault(processId);

    EXPECT_EQ(left, right);
    EXPECT_FALSE(left != right);
}

} // namespace

} // namespace emmus::test
