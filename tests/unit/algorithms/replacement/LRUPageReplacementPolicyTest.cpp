#include <array>
#include <optional>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/LRUPageReplacementPolicy.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp" 

namespace emmus::test
{

class LRUPageReplacementPolicyTest
    : public PageReplacementAlgorithmFixture
{
protected:

    using Policy =
        emmus::algorithms::replacement::LRUPageReplacementPolicy;

    using Statistics =
        emmus::statistics::PageReplacementStatistics;


    Policy policy_;
};


/*
 * --------------------------------------------------------------------------
 * Empty state
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    ChooseVictimReturnsNulloptWhenNoPagesAreTracked
)
{
    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );

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
}


/*
 * --------------------------------------------------------------------------
 * Page loading
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    LoadedPageBecomesTheOnlyVictim
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    LoadedPagesAreInitiallyOrderedByLoadTime
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RepeatedLoadOfSamePageAndFrameIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageLoaded(kPage0, kFrame0);


    const auto firstVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(firstVictim.has_value());

    EXPECT_EQ(
        firstVictim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    LoadingSamePageIntoDifferentFrameIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame1);


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    LoadingDifferentPageIntoSameFrameIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame0);


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


/*
 * --------------------------------------------------------------------------
 * Access ordering
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    AccessingLeastRecentlyUsedPageMakesItMostRecentlyUsed
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    /*
     * Initial order:
     *
     * LRU -> Page0, Page1, Page2 <- MRU
     *
     * Accessing Page0 produces:
     *
     * LRU -> Page1, Page2, Page0 <- MRU
     */
    policy_.pageAccessed(
        kPage0,
        kFrame0
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame1
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    AccessingMiddlePageMakesItMostRecentlyUsed
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    /*
     * Initial:
     * Page0, Page1, Page2
     *
     * Access Page1:
     * Page0, Page2, Page1
     */
    policy_.pageAccessed(
        kPage1,
        kFrame1
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    AccessingMostRecentlyUsedPageDoesNotChangeVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    policy_.pageAccessed(
        kPage2,
        kFrame2
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RepeatedAccessesUpdateRecencyEachTime
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    /*
     * Access sequence:
     *
     * Initial:       0, 1, 2
     * Access 0:      1, 2, 0
     * Access 1:      2, 0, 1
     *
     * Therefore frame 2 is the LRU victim.
     */
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage1, kFrame1);


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame2
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    AccessSequenceProducesDeterministicVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);
    policy_.pageLoaded(kPage3, kFrame3);


    /*
     * Initial:
     * 0, 1, 2, 3
     *
     * Access 0:
     * 1, 2, 3, 0
     *
     * Access 2:
     * 1, 3, 0, 2
     *
     * Access 1:
     * 3, 0, 2, 1
     *
     * Frame 3 is now LRU.
     */
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage2, kFrame2);
    policy_.pageAccessed(kPage1, kFrame1);


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame3
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    AccessToUntrackedPageDoesNotChangeVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    policy_.pageAccessed(
        kPage2,
        kFrame2
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    AccessWithMismatchedFrameDoesNotChangeRecency
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    policy_.pageAccessed(
        kPage0,
        kFrame1
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


/*
 * --------------------------------------------------------------------------
 * Victim selection
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    ChooseVictimDoesNotRemoveSelectedEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    const auto firstVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(firstVictim.has_value());

    EXPECT_EQ(
        firstVictim.value(),
        kFrame0
    );


    /*
     * The caller has not yet reported page removal, so the
     * same entry remains the LRU victim.
     */
    const auto secondVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(secondVictim.has_value());

    EXPECT_EQ(
        secondVictim.value(),
        kFrame0
    );


    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        2U
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingVictimAllowsNextLeastRecentlyUsedPageToBeSelected
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    const auto firstVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(firstVictim.has_value());

    EXPECT_EQ(
        firstVictim.value(),
        kFrame0
    );


    policy_.pageRemoved(
        kPage0,
        kFrame0
    );


    const auto secondVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(secondVictim.has_value());

    EXPECT_EQ(
        secondVictim.value(),
        kFrame1
    );
}


/*
 * --------------------------------------------------------------------------
 * Removal
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingLeastRecentlyUsedPageRemovesItFromTracking
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    policy_.pageRemoved(
        kPage0,
        kFrame0
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame1
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingMiddlePagePreservesRemainingOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    policy_.pageRemoved(
        kPage1,
        kFrame1
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingMostRecentlyUsedPagePreservesRemainingOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    policy_.pageRemoved(
        kPage2,
        kFrame2
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingUntrackedPageDoesNotChangeVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    policy_.pageRemoved(
        kPage2,
        kFrame2
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingPageWithMismatchedFrameDoesNotRemoveEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    policy_.pageRemoved(
        kPage0,
        kFrame1
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingAllPagesProducesEmptyPolicy
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageRemoved(kPage1, kFrame1);


    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );
}


/*
 * --------------------------------------------------------------------------
 * Replacement cycles
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    ReplacementCycleMaintainsCorrectLruOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);


    /*
     * Initial:
     * 0, 1, 2
     */
    policy_.pageAccessed(kPage0, kFrame0);


    /*
     * Order:
     * 1, 2, 0
     */
    const auto firstVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(firstVictim.has_value());

    EXPECT_EQ(
        firstVictim.value(),
        kFrame1
    );


    policy_.pageRemoved(kPage1, kFrame1);


    /*
     * Reuse frame 1 for page 3.
     *
     * New order:
     * 2, 0, 3
     */
    policy_.pageLoaded(kPage3, kFrame1);


    const auto secondVictim =
        policy_.chooseVictim();


    ASSERT_TRUE(secondVictim.has_value());

    EXPECT_EQ(
        secondVictim.value(),
        kFrame2
    );
}


/*
 * --------------------------------------------------------------------------
 * Reset
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    ResetClearsResidencyTracking
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);


    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );


    policy_.reset();


    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    ResetClearsStatistics
)
{
    policy_.pageLoaded(kPage0, kFrame0);


    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
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
}


/*
 * --------------------------------------------------------------------------
 * Statistics
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    SuccessfulVictimSelectionRecordsReplacement
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );


    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );


    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        1U
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    RepeatedVictimSelectionsRecordEachReplacementDecision
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );


    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );


    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        3U
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    EmptyVictimSelectionDoesNotRecordReplacement
)
{
    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );


    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    SuccessfulVictimSelectionRecordsExecutionTime
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );


    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );


    EXPECT_GE(
        policy_.statistics().totalExecutionTime().count(),
        0
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    EmptyVictimSelectionDoesNotRecordExecutionTime
)
{
    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );


    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        Statistics::Duration::zero()
    );
}


TEST_F(
    LRUPageReplacementPolicyTest,
    ReplacementStatisticsRemainIndependentOfDirtyEvictionState
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );


    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );


    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        1U
    );

    EXPECT_EQ(
        policy_.statistics().dirtyEvictionCount(),
        0U
    );
}


/*
 * --------------------------------------------------------------------------
 * Fixed reference sequences
 * --------------------------------------------------------------------------
 */

TEST_F(
    LRUPageReplacementPolicyTest,
    FixedReferenceSequenceSelectsExpectedVictim
)
{
    constexpr std::array<PageId, 5> referencePages{
        kPage0,
        kPage1,
        kPage2,
        kPage0,
        kPage3
    };


    constexpr std::array<FrameId, 4> referenceFrames{
        kFrame0,
        kFrame1,
        kFrame2,
        kFrame3
    };


    policy_.pageLoaded(
        referencePages[0],
        referenceFrames[0]
    );

    policy_.pageLoaded(
        referencePages[1],
        referenceFrames[1]
    );

    policy_.pageLoaded(
        referencePages[2],
        referenceFrames[2]
    );

    /*
     * Access page 0:
     *
     * Page1, Page2, Page0
     */
    policy_.pageAccessed(
        referencePages[3],
        referenceFrames[0]
    );


    /*
     * Load page 3:
     *
     * Page1, Page2, Page0, Page3
     */
    policy_.pageLoaded(
        referencePages[4],
        referenceFrames[3]
    );


    const auto victim =
        policy_.chooseVictim();


    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        victim.value(),
        kFrame1
    );
}

} // namespace emmus::test