#include "emmus/statistics/PageReplacementStatistics.hpp"

#include <chrono>

#include <gtest/gtest.h>

namespace emmus::test
{

namespace
{

using Statistics =
    emmus::statistics::PageReplacementStatistics;


class PageReplacementStatisticsTest
    : public ::testing::Test
{
protected:

    Statistics statistics;
};


/**
 * @brief A newly constructed statistics object contains no measurements.
 */
TEST_F(
    PageReplacementStatisticsTest,
    InitialStatisticsAreZero
)
{
    EXPECT_EQ(
        statistics.replacementCount(),
        0U
    );

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration::zero()
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.0
    );
}


/**
 * @brief A replacement event increments the replacement counter.
 */
TEST_F(
    PageReplacementStatisticsTest,
    RecordReplacementIncrementsReplacementCount
)
{
    statistics.recordReplacement();

    EXPECT_EQ(
        statistics.replacementCount(),
        1U
    );
}


/**
 * @brief Multiple replacement events are accumulated.
 */
TEST_F(
    PageReplacementStatisticsTest,
    MultipleReplacementsAreAccumulated
)
{
    statistics.recordReplacement();
    statistics.recordReplacement();
    statistics.recordReplacement();

    EXPECT_EQ(
        statistics.replacementCount(),
        3U
    );
}


/**
 * @brief A dirty eviction increments the dirty-eviction counter.
 */
TEST_F(
    PageReplacementStatisticsTest,
    RecordDirtyEvictionIncrementsDirtyEvictionCount
)
{
    statistics.recordDirtyEviction();

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        1U
    );
}


/**
 * @brief Multiple dirty evictions are accumulated.
 */
TEST_F(
    PageReplacementStatisticsTest,
    MultipleDirtyEvictionsAreAccumulated
)
{
    statistics.recordDirtyEviction();
    statistics.recordDirtyEviction();
    statistics.recordDirtyEviction();

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        3U
    );
}


/**
 * @brief Dirty-page eviction statistics are independent of
 *        replacement statistics.
 */
TEST_F(
    PageReplacementStatisticsTest,
    DirtyEvictionDoesNotIncrementReplacementCount
)
{
    statistics.recordDirtyEviction();

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        1U
    );

    EXPECT_EQ(
        statistics.replacementCount(),
        0U
    );
}


/**
 * @brief Replacement statistics are independent of dirty-page
 *        eviction statistics.
 */
TEST_F(
    PageReplacementStatisticsTest,
    ReplacementDoesNotIncrementDirtyEvictionCount
)
{
    statistics.recordReplacement();

    EXPECT_EQ(
        statistics.replacementCount(),
        1U
    );

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        0U
    );
}


/**
 * @brief Execution time is accumulated using std::chrono durations.
 */
TEST_F(
    PageReplacementStatisticsTest,
    RecordExecutionTimeStoresDuration
)
{
    using namespace std::chrono_literals;

    statistics.recordExecutionTime(
        250us
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration{250us}
    );
}


/**
 * @brief Multiple execution durations are accumulated.
 */
TEST_F(
    PageReplacementStatisticsTest,
    ExecutionTimesAreAccumulated
)
{
    using namespace std::chrono_literals;

    statistics.recordExecutionTime(
        100us
    );

    statistics.recordExecutionTime(
        250us
    );

    statistics.recordExecutionTime(
        650us
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration{1000us}
    );
}


/**
 * @brief Recording execution time does not itself constitute
 *        a replacement event.
 */
TEST_F(
    PageReplacementStatisticsTest,
    RecordingExecutionTimeDoesNotIncrementReplacementCount
)
{
    using namespace std::chrono_literals;

    statistics.recordExecutionTime(
        500us
    );

    EXPECT_EQ(
        statistics.replacementCount(),
        0U
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration{500us}
    );
}


/**
 * @brief The average is zero when there are no replacements.
 */
TEST_F(
    PageReplacementStatisticsTest,
    AverageExecutionTimeIsZeroWhenThereAreNoReplacements
)
{
    using namespace std::chrono_literals;

    statistics.recordExecutionTime(
        500us
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.0
    );
}


/**
 * @brief One replacement produces an average equal to its
 *        recorded execution time.
 */
TEST_F(
    PageReplacementStatisticsTest,
    AverageExecutionTimeIsCorrectForOneReplacement
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();

    statistics.recordExecutionTime(
        500us
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.5
    );
}


/**
 * @brief The average is calculated across multiple replacements.
 */
TEST_F(
    PageReplacementStatisticsTest,
    AverageExecutionTimeIsCorrectForMultipleReplacements
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();
    statistics.recordExecutionTime(100us);

    statistics.recordReplacement();
    statistics.recordExecutionTime(300us);

    statistics.recordReplacement();
    statistics.recordExecutionTime(500us);

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.3
    );
}


/**
 * @brief Fractional millisecond averages are preserved.
 */
TEST_F(
    PageReplacementStatisticsTest,
    AverageExecutionTimePreservesFractionalMilliseconds
)
{
    statistics.recordReplacement();

    statistics.recordExecutionTime(
        Statistics::Duration{1}
    );

    statistics.recordReplacement();

    statistics.recordExecutionTime(
        Statistics::Duration{2}
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.0000015
    );
}


/**
 * @brief A zero-duration replacement is valid and produces a
 *        zero average.
 */
TEST_F(
    PageReplacementStatisticsTest,
    ZeroDurationIsHandledCorrectly
)
{
    statistics.recordReplacement();

    statistics.recordExecutionTime(
        Statistics::Duration::zero()
    );

    EXPECT_EQ(
        statistics.replacementCount(),
        1U
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration::zero()
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.0
    );
}


/**
 * @brief All statistics can coexist independently.
 */
TEST_F(
    PageReplacementStatisticsTest,
    StatisticsAreMaintainedIndependently
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();
    statistics.recordReplacement();

    statistics.recordDirtyEviction();

    statistics.recordExecutionTime(
        750us
    );

    EXPECT_EQ(
        statistics.replacementCount(),
        2U
    );

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        1U
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration{750us}
    );
}


/**
 * @brief Reset restores all statistics to their initial state.
 */
TEST_F(
    PageReplacementStatisticsTest,
    ResetRestoresInitialState
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();
    statistics.recordReplacement();

    statistics.recordDirtyEviction();
    statistics.recordDirtyEviction();

    statistics.recordExecutionTime(
        1250us
    );

    statistics.reset();

    EXPECT_EQ(
        statistics.replacementCount(),
        0U
    );

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration::zero()
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.0
    );
}


/**
 * @brief Statistics can be collected again after reset.
 */
TEST_F(
    PageReplacementStatisticsTest,
    StatisticsCanBeReusedAfterReset
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();

    statistics.recordDirtyEviction();

    statistics.recordExecutionTime(
        100us
    );

    statistics.reset();

    statistics.recordReplacement();

    statistics.recordExecutionTime(
        900us
    );

    EXPECT_EQ(
        statistics.replacementCount(),
        1U
    );

    EXPECT_EQ(
        statistics.dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        statistics.totalExecutionTime(),
        Statistics::Duration{900us}
    );

    EXPECT_DOUBLE_EQ(
        statistics.averageExecutionTimeMilliseconds(),
        0.9
    );
}


/**
 * @brief A statistics object is copyable without sharing state.
 */
TEST_F(
    PageReplacementStatisticsTest,
    CopyProducesIndependentStatistics
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();

    statistics.recordDirtyEviction();

    statistics.recordExecutionTime(
        500us
    );

    const Statistics copy =
        statistics;

    statistics.recordReplacement();

    EXPECT_EQ(
        copy.replacementCount(),
        1U
    );

    EXPECT_EQ(
        copy.dirtyEvictionCount(),
        1U
    );

    EXPECT_EQ(
        copy.totalExecutionTime(),
        Statistics::Duration{500us}
    );
}


/**
 * @brief Move construction preserves the collected statistics.
 */
TEST_F(
    PageReplacementStatisticsTest,
    MoveConstructionPreservesStatistics
)
{
    using namespace std::chrono_literals;

    statistics.recordReplacement();

    statistics.recordDirtyEviction();

    statistics.recordExecutionTime(
        500us
    );

    Statistics movedStatistics =
        std::move(statistics);

    EXPECT_EQ(
        movedStatistics.replacementCount(),
        1U
    );

    EXPECT_EQ(
        movedStatistics.dirtyEvictionCount(),
        1U
    );

    EXPECT_EQ(
        movedStatistics.totalExecutionTime(),
        Statistics::Duration{500us}
    );
}

} // namespace

} // namespace emmus::test