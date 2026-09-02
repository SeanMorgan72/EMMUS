#include <optional>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"

namespace emmus::test
{

class FIFOPageReplacementPolicyTest
    : public PageReplacementAlgorithmFixture
{
protected:

    emmus::algorithms::replacement::FIFOPageReplacementPolicy policy_;
};


// ============================================================================
// Empty State
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    ChooseVictimOnEmptyPolicyReturnsNullopt
)
{
    EXPECT_EQ(
        policy_.chooseVictim(),
        std::nullopt
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );
}


// ============================================================================
// Single Page
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    FirstLoadedFrameIsSelectedAsVictim
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        1U
    );
}


// ============================================================================
// FIFO Ordering
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    VictimSelectionFollowsLoadOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    RepeatedVictimSelectionReturnsSameOldestFrameUntilRemoved
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


// ============================================================================
// Page Access
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    PageAccessDoesNotChangeFIFOOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(
        kPage2,
        kFrame2
    );

    policy_.pageAccessed(
        kPage1,
        kFrame1
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


// ============================================================================
// Removal
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    RemovingOldestFrameMakesNextFrameTheVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    RemovingMiddleFramePreservesRemainingFIFOOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageRemoved(
        kPage1,
        kFrame1
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame2
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    RemovingNewestFrameLeavesOlderFramesInOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageRemoved(
        kPage2,
        kFrame2
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    RemovingUntrackedPageDoesNotChangeFIFOState
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageRemoved(
        kPage3,
        kFrame3
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


// ============================================================================
// Replacement Cycles
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    LoadingPageAfterRemovalAddsItToEndOfFIFOOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage2,
        kFrame2
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    policy_.pageRemoved(
        kPage1,
        kFrame1
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame2
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    MultipleReplacementCyclesRemainDeterministic
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageLoaded(kPage3, kFrame3);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage0, kFrame0);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame2
    );
}


// ============================================================================
// Duplicate / Invalid Notifications
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    DuplicateLoadDoesNotDuplicateFIFOEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame0);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        std::nullopt
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    DuplicateLoadOfSamePageWithDifferentFrameDoesNotCreateSecondEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame1);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    DuplicateLoadOfSameFrameWithDifferentPageDoesNotCreateSecondEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame0);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    RemovingWithMismatchedPageAndFrameDoesNotRemoveEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageRemoved(
        kPage0,
        kFrame1
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


// ============================================================================
// Reset
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    ResetClearsFIFOState
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.reset();

    EXPECT_EQ(
        policy_.chooseVictim(),
        std::nullopt
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    ResetClearsStatistics
)
{
    policy_.pageLoaded(kPage0, kFrame0);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        1U
    );

    policy_.reset();

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        policy_.statistics().dirtyEvictionCount(),
        0U
    );

    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        emmus::statistics::PageReplacementStatistics::Duration::zero()
    );

    EXPECT_DOUBLE_EQ(
        policy_.statistics().averageExecutionTimeMilliseconds(),
        0.0
    );
}


// ============================================================================
// Statistics
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    EachSuccessfulVictimSelectionRecordsOneReplacement
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        1U
    );

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        2U
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    EmptyVictimSelectionDoesNotRecordReplacement
)
{
    EXPECT_EQ(
        policy_.chooseVictim(),
        std::nullopt
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        emmus::statistics::PageReplacementStatistics::Duration::zero()
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    VictimSelectionRecordsExecutionTime
)
{
    policy_.pageLoaded(kPage0, kFrame0);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_GE(
        policy_.statistics().totalExecutionTime().count(),
        0
    );

    EXPECT_GE(
        policy_.statistics().averageExecutionTimeMilliseconds(),
        0.0
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    FIFODoesNotRecordDirtyEvictions
)
{
    policy_.pageLoaded(kPage0, kFrame0);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    EXPECT_EQ(
        policy_.statistics().dirtyEvictionCount(),
        0U
    );
}

} // namespace emmus::test