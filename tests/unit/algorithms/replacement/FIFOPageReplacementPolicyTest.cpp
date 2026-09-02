#include <array>
#include <optional>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/FIFOPageReplacementPolicy.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"

namespace emmus::test
{

class FIFOPageReplacementPolicyTest
    : public PageReplacementAlgorithmFixture
{
protected:

    using FIFOPageReplacementPolicy =
        emmus::algorithms::replacement::FIFOPageReplacementPolicy;

    using Statistics =
        emmus::statistics::PageReplacementStatistics;

    FIFOPageReplacementPolicy policy_;
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

    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        Statistics::Duration::zero()
    );
}


// ============================================================================
// Single Frame
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    SingleFrameAlwaysSelectsOnlyResidentFrame
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
    SingleFrameReplacementCycleRemainsDeterministic
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    ASSERT_EQ(
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

    policy_.pageLoaded(
        kPage1,
        kFrame0
    );

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
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
// Basic FIFO Ordering
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    VictimSelectionFollowsLoadOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    // Expected FIFO victim sequence:
    //
    //   Load order:     F0 -> F1 -> F2
    //   First victim:   F0
    //
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
// US-402: Fixed Reference Sequence Tests
// ============================================================================
//
// These tests use fixed page/frame reference sequences and explicit victim
// sequences as the test oracle.
//
// FIFO ordering is determined solely by residency/load order.
// A page access must never promote a page or otherwise alter that order.
//
// ============================================================================

TEST_F(
    FIFOPageReplacementPolicyTest,
    ReferenceSequenceProducesExpectedFIFOVictimSequence
)
{
    // Fixed reference sequence:
    //   Load: Page 0 -> Frame 0
    //   Load: Page 1 -> Frame 1
    //   Load: Page 2 -> Frame 2
    //
    // Expected FIFO victim sequence:
    //   1. Frame 0
    //   2. Frame 1
    //   3. Frame 2

    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    // First replacement: oldest loaded page is Page 0 / Frame 0.
    {
        const auto victim = policy_.chooseVictim();

        ASSERT_TRUE(victim.has_value());
        EXPECT_EQ(*victim, kFrame0);
    }

    // Remove the first victim so Frame 1 becomes the oldest resident frame.
    policy_.pageRemoved(kPage0, kFrame0);

    // Second replacement: Page 1 / Frame 1.
    {
        const auto victim = policy_.chooseVictim();

        ASSERT_TRUE(victim.has_value());
        EXPECT_EQ(*victim, kFrame1);
    }

    // Remove the second victim so Frame 2 becomes the oldest resident frame.
    policy_.pageRemoved(kPage1, kFrame1);

    // Third replacement: Page 2 / Frame 2.
    {
        const auto victim = policy_.chooseVictim();

        ASSERT_TRUE(victim.has_value());
        EXPECT_EQ(*victim, kFrame2);
    }

    // Remove the final resident page.
    policy_.pageRemoved(kPage2, kFrame2);

    // No resident pages remain, so no victim should be available.
    EXPECT_EQ(policy_.chooseVictim(), std::nullopt);
}

TEST_F(
    FIFOPageReplacementPolicyTest,
    ReferenceSequenceWithRepeatedAccessesPreservesExpectedVictimSequence
)
{
    // ------------------------------------------------------------------------
    // Fixed resident sequence:
    //
    //   P0/F0 -> P1/F1 -> P2/F2
    //
    // Reference/access sequence:
    //
    //   P2, P0, P1, P0, P2
    //
    // Expected FIFO victim sequence remains:
    //
    //   F0 -> F1 -> F2
    //
    // The access pattern intentionally favors pages that were loaded first
    // and last. FIFO must ignore those accesses when determining victims.
    // ------------------------------------------------------------------------

    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage2, kFrame2);
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage1, kFrame1);
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage2, kFrame2);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage0, kFrame0);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    policy_.pageRemoved(kPage1, kFrame1);

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame2
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    CompleteReplacementReferenceSequenceProducesExpectedVictims
)
{
    // ------------------------------------------------------------------------
    // Fixed three-frame FIFO reference sequence.
    //
    // Initial loads:
    //
    //   P0/F0 -> P1/F1 -> P2/F2
    //
    // Replacement sequence:
    //
    //   1. Evict F0, load P3 into F0
    //   2. Evict F1, load P0 into F1
    //   3. Evict F2, load P1 into F2
    //   4. Evict F0, load P2 into F0
    //   5. Evict F1
    //
    // Expected victim sequence:
    //
    //   F0 -> F1 -> F2 -> F0 -> F1
    //
    // This verifies that newly loaded pages are appended to the back of
    // the FIFO residency order.
    // ------------------------------------------------------------------------

    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    // Replacement 1: F0
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageLoaded(kPage3, kFrame0);

    // Replacement 2: F1
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage0, kFrame1);

    // Replacement 3: F2
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame2
    );

    policy_.pageRemoved(kPage2, kFrame2);
    policy_.pageLoaded(kPage1, kFrame2);

    // Replacement 4: F0
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage3, kFrame0);
    policy_.pageLoaded(kPage2, kFrame0);

    // Replacement 5: F1
    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        5U
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    FrequentReplacementReferenceSequenceRemainsFIFO
)
{
    // ------------------------------------------------------------------------
    // Two-frame configuration:
    //
    // Initial load order:
    //
    //   P0/F0 -> P1/F1
    //
    // Repeated replacement sequence:
    //
    //   F0 -> F1 -> F0 -> F1
    //
    // Expected victim sequence:
    //
    //   F0 -> F1 -> F0 -> F1
    //
    // Each replacement reloads the released frame with a new page. This
    // verifies that every newly loaded frame is appended to the FIFO queue.
    // ------------------------------------------------------------------------

    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    // Replacement 1: F0
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageLoaded(kPage2, kFrame0);

    // Replacement 2: F1
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage3, kFrame1);

    // Replacement 3: F0
    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage2, kFrame0);
    policy_.pageLoaded(kPage0, kFrame0);

    // Replacement 4: F1
    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        4U
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


TEST_F(
    FIFOPageReplacementPolicyTest,
    RepeatedAccessToNewestPageDoesNotPromoteItAheadOfOlderPages
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    for (int i = 0; i < 10; ++i)
    {
        policy_.pageAccessed(
            kPage2,
            kFrame2
        );
    }

    EXPECT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );
}


TEST_F(
    FIFOPageReplacementPolicyTest,
    RepeatedAccessToOldestPageDoesNotChangeItsFIFOPosition
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    for (int i = 0; i < 10; ++i)
    {
        policy_.pageAccessed(
            kPage0,
            kFrame0
        );
    }

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


TEST_F(
    FIFOPageReplacementPolicyTest,
    RemovingAllPagesLeavesPolicyWithoutVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageRemoved(kPage1, kFrame1);

    EXPECT_EQ(
        policy_.chooseVictim(),
        std::nullopt
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

    // Expected victim sequence:
    //
    //   F0 -> F1 -> F2
    //
    // with each released frame reused for the newly loaded page.

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
        Statistics::Duration::zero()
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
        Statistics::Duration::zero()
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


TEST_F(
    FIFOPageReplacementPolicyTest,
    ReplacementStatisticsMatchCompleteReferenceSequence
)
{
    // Expected victim sequence:
    //
    //   F0 -> F1 -> F2 -> F0 -> F1
    //
    // Therefore five successful calls to chooseVictim() are expected.

    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageLoaded(kPage3, kFrame0);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage0, kFrame1);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame2
    );

    policy_.pageRemoved(kPage2, kFrame2);
    policy_.pageLoaded(kPage1, kFrame2);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame0
    );

    policy_.pageRemoved(kPage3, kFrame0);
    policy_.pageLoaded(kPage2, kFrame0);

    ASSERT_EQ(
        policy_.chooseVictim(),
        kFrame1
    );

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        5U
    );

    EXPECT_EQ(
        policy_.statistics().dirtyEvictionCount(),
        0U
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

} // namespace emmus::test