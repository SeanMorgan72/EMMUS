#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/ClockPageReplacementPolicy.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"

namespace emmus::test
{

class ClockPageReplacementPolicyTest
    : public PageReplacementAlgorithmFixture
{
protected:

    using Policy =
        emmus::algorithms::replacement::ClockPageReplacementPolicy;

    using Statistics =
        emmus::statistics::PageReplacementStatistics;

    Policy policy_;
};


// ============================================================================
// Empty State
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    EmptyPolicyReturnsNoVictim
)
{
    EXPECT_FALSE(policy_.chooseVictim().has_value());
}


TEST_F(
    ClockPageReplacementPolicyTest,
    EmptyPolicyDoesNotRecordReplacement
)
{
    const auto victim =
        policy_.chooseVictim();

    EXPECT_FALSE(victim.has_value());

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );

    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        std::chrono::nanoseconds::zero()
    );
}


// ============================================================================
// Single Page / Frame
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    SingleLoadedPageIsEventuallySelected
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
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    SinglePageReceivesSecondChanceBeforeSelection
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    /*
     * The page starts referenced because it was loaded.
     *
     * The first selection clears the reference bit and advances
     * the hand around the one-entry circular collection.
     */
    const auto firstVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());
    EXPECT_EQ(*firstVictim, kFrame0);

    /*
     * The second selection finds the now-cleared reference bit.
     */
    const auto secondVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(secondVictim.has_value());
    EXPECT_EQ(*secondVictim, kFrame0);
}


// ============================================================================
// Page Loading
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    LoadedPagesAreTracked
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * All newly loaded entries have their reference bit set.
     * The first complete scan clears them, then F0 is selected.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    DuplicatePageLoadDoesNotCreateDuplicateEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame0);

    const auto firstVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());

    EXPECT_EQ(
        *firstVictim,
        kFrame0
    );

    /*
     * Only one entry exists, so the next victim is still F0.
     */
    const auto secondVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(secondVictim.has_value());

    EXPECT_EQ(
        *secondVictim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ConflictingPageLoadDoesNotReplaceExistingMapping
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame1);

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ConflictingFrameLoadDoesNotReplaceExistingMapping
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame0);

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


// ============================================================================
// Reference Bit / Access
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    AccessSetsReferenceBit
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    /*
     * First selection:
     * F0 = referenced -> clear and advance
     * F1 = referenced -> clear and advance
     * F0 = clear -> select F0
     *
     * Hand now points to F1.
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    /*
     * Re-reference F0. The hand starts at F1:
     *
     * F1 = clear -> select F1
     *
     * Hand moves to F0.
     */
    policy_.pageAccessed(kPage0, kFrame0);

    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * F0 was referenced after the previous selection, so when the
     * hand reaches it, it receives a second chance and is skipped.
     *
     * F1 is currently clear and is therefore selected.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);
}


TEST_F(
    ClockPageReplacementPolicyTest,
    RepeatedAccessKeepsReferenceBitSet
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    /*
     * Clear the initial reference bits and establish the hand at F1.
     */
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage0, kFrame0);

    /*
     * Hand begins at F1, whose bit is clear, so F1 is selected.
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * Hand now points at F0. Its reference bit is set, so F0 gets
     * a second chance and is cleared before F1 is selected.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);
}


TEST_F(
    ClockPageReplacementPolicyTest,
    AccessToUnknownPageIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageAccessed(kPage2, kFrame2);

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    AccessWithMismatchedFrameIsIgnored
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
        *victim,
        kFrame0
    );
}


// ============================================================================
// Second-Chance Algorithm
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    ReferencedPageReceivesSecondChance
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * All three entries begin referenced.
     *
     * F0 gets a second chance and is cleared.
     * F1 gets a second chance and is cleared.
     * F2 gets a second chance and is cleared.
     * The hand wraps to F0, which is now clear and is selected.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ClearedReferenceBitEntryIsSelected
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * First selection clears every reference bit and selects F0.
     * The hand advances to F1.
     */
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    /*
     * F1 now has a clear reference bit, so it is selected immediately.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame1
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ClockHandAdvancesAfterVictimSelection
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * First decision clears all bits and selects F0.
     * The hand must advance to F1.
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    /*
     * F1 is clear and should therefore be selected next.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * The hand advances to F2.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ClockHandWrapsAroundCircularCollection
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * Victim sequence with no new accesses:
     *
     * F0 -> F1 -> F2 -> F0
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);

    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}


TEST_F(
    ClockPageReplacementPolicyTest,
    AccessedPageCanReceiveSecondChanceAfterHandWrap
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    ASSERT_TRUE(policy_.chooseVictim().has_value());

    /*
     * Hand is now at F1. Reference F0 before the hand reaches it
     * again.
     */
    policy_.pageAccessed(kPage0, kFrame0);

    /*
     * F1 is clear -> selected.
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * Hand is now F2. F2 is clear -> selected.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);

    /*
     * Hand wraps to F0. F0 was referenced, so it gets a second
     * chance. The scan continues to F1, whose bit is clear.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * The test verifies that the referenced page was not selected
     * immediately when first encountered.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);
}


// ============================================================================
// Victim Persistence
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    ChooseVictimDoesNotRemovePage
)
{
    policy_.pageLoaded(kPage0, kFrame0);

    const auto firstVictim =
        policy_.chooseVictim();

    const auto secondVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());
    ASSERT_TRUE(secondVictim.has_value());

    EXPECT_EQ(
        *firstVictim,
        kFrame0
    );

    EXPECT_EQ(
        *secondVictim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    VictimIsRemovedOnlyAfterPageRemovedNotification
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    const auto firstVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());
    EXPECT_EQ(*firstVictim, kFrame0);

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    const auto secondVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(secondVictim.has_value());
    EXPECT_EQ(*secondVictim, kFrame1);
}


// ============================================================================
// Page Removal
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    RemovingCurrentHandEntryPreservesCircularTraversal
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * First selection clears all bits and selects F0.
     * The hand is now F1.
     */
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    policy_.pageRemoved(
        kPage1,
        kFrame1
    );

    /*
     * The hand now points at the entry that followed F1, namely F2.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame2
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    RemovingEntryBeforeClockHandAdjustsHand
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * First selection:
     * victim F0, hand -> F1.
     */
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    /*
     * Remove the entry before the hand: F0.
     *
     * The hand must remain logically on F1 rather than move to F2.
     */
    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame1
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    RemovingLastEntryWrapsClockHand
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * Clear all reference bits and select F0, F1, F2 in sequence.
     * After selecting F2 the hand wraps to F0.
     */
    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    policy_.pageRemoved(
        kPage2,
        kFrame2
    );

    /*
     * The hand was logically at F0 after F2 was selected.
     * Removing F2 must preserve that position.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    RemovingUnknownPageIsIgnored
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
        *victim,
        kFrame0
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    RemovingPageWithMismatchedFrameIsIgnored
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
        *victim,
        kFrame0
    );
}


// ============================================================================
// Replacement Cycles
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    ReplacementCycleMaintainsClockOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * Initial selection:
     * all pages receive a second chance,
     * then F0 is selected.
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage3,
        kFrame0
    );

    /*
     * The clock hand is at F1.
     *
     * F1 is clear, so it is selected next.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ReplacementCycleAllowsAccessedPageToReceiveSecondChance
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * Initial selection clears the reference bits and selects F0.
     * Hand -> F1.
     */
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    /*
     * Re-reference F2.
     */
    policy_.pageAccessed(
        kPage2,
        kFrame2
    );

    /*
     * F1 is clear -> selected.
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * Hand -> F2.
     * F2 is referenced, so it receives a second chance.
     *
     * Next F0 is clear and is selected.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    /*
     * The selected victim remains tracked, as required by the
     * common replacement-policy contract.
     */
}


// ============================================================================
// Reset
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    ResetClearsTrackedPages
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.reset();

    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ResetClearsClockState
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * Move the clock state away from its initial position.
     */
    ASSERT_TRUE(policy_.chooseVictim().has_value());
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    policy_.reset();

    /*
     * Loading after reset must establish a fresh Clock state.
     */
    policy_.pageLoaded(
        kPage3,
        kFrame3
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame3
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    ResetClearsStatistics
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    EXPECT_GT(
        policy_.statistics().replacementCount(),
        0U
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
        std::chrono::nanoseconds::zero()
    );
}


// ============================================================================
// Statistics
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    SuccessfulVictimSelectionRecordsReplacement
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        1U
    );
}


TEST_F(
    ClockPageReplacementPolicyTest,
    RepeatedVictimSelectionRecordsEachSelection
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

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
    ClockPageReplacementPolicyTest,
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
    ClockPageReplacementPolicyTest,
    ClockDoesNotRecordDirtyEvictions
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    EXPECT_EQ(
        policy_.statistics().dirtyEvictionCount(),
        0U
    );
}


// ============================================================================
// Deterministic Reference Sequences
// ============================================================================

struct ClockReferenceSequenceCase
{
    std::string name;

    std::vector<
        emmus::memory::identifiers::PageId
    > initialPages;

    std::vector<
        emmus::memory::identifiers::FrameId
    > initialFrames;

    std::vector<
        emmus::memory::identifiers::PageId
    > accessPages;

    std::vector<
        emmus::memory::identifiers::FrameId
    > accessFrames;

    emmus::memory::identifiers::FrameId expectedVictim;
};


class ClockReferenceSequenceTest
    : public ClockPageReplacementPolicyTest,
      public ::testing::WithParamInterface<
          ClockReferenceSequenceCase
      >
{
};


TEST_P(
    ClockReferenceSequenceTest,
    ReferenceSequenceProducesExpectedVictim
)
{
    const auto& testCase =
        GetParam();

    ASSERT_EQ(
        testCase.initialPages.size(),
        testCase.initialFrames.size()
    );

    ASSERT_EQ(
        testCase.accessPages.size(),
        testCase.accessFrames.size()
    );

    for (
        std::size_t index{0U};
        index < testCase.initialPages.size();
        ++index
    )
    {
        policy_.pageLoaded(
            testCase.initialPages[index],
            testCase.initialFrames[index]
        );
    }

    for (
        std::size_t index{0U};
        index < testCase.accessPages.size();
        ++index
    )
    {
        policy_.pageAccessed(
            testCase.accessPages[index],
            testCase.accessFrames[index]
        );
    }

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        testCase.expectedVictim
    );
}


INSTANTIATE_TEST_SUITE_P(
    DeterministicReferenceSequences,
    ClockReferenceSequenceTest,
    ::testing::Values(

        /*
         * All newly loaded pages begin referenced.
         *
         * F0 -> second chance
         * F1 -> second chance
         * F2 -> second chance
         * F0 -> clear bit -> victim
         */
        ClockReferenceSequenceCase{
            "InitialLoadOrder",
            {
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{2}
            },
            {},
            {},
            emmus::memory::identifiers::FrameId{0}
        },

        /*
         * Accessing F0 does not change the result because F0 is already
         * referenced when initially loaded.
         */
        ClockReferenceSequenceCase{
            "AccessInitiallyReferencedPage",
            {
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{2}
            },
            {
                emmus::memory::identifiers::PageId{0}
            },
            {
                emmus::memory::identifiers::FrameId{0}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        /*
         * After the first scan:
         *
         * victim F0
         * hand -> F1
         *
         * F0 is then accessed, but this test only performs one
         * victim selection, so F0 remains the first selected victim.
         */
        ClockReferenceSequenceCase{
            "RepeatedReferenceBeforeFirstSelection",
            {
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{2}
            },
            {
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{0}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{0}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        /*
         * Unknown accesses are ignored.
         */
        ClockReferenceSequenceCase{
            "UnknownAccessIgnored",
            {
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{2}
            },
            {
                emmus::memory::identifiers::PageId{3}
            },
            {
                emmus::memory::identifiers::FrameId{3}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        /*
         * Mixed accesses before the first selection do not affect
         * the initial all-referenced state.
         */
        ClockReferenceSequenceCase{
            "MixedInitialReferences",
            {
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{3}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{3}
            },
            {
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{3},
                emmus::memory::identifiers::PageId{1}
            },
            {
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{3},
                emmus::memory::identifiers::FrameId{1}
            },
            emmus::memory::identifiers::FrameId{0}
        }
    ),
    [](const ::testing::TestParamInfo<ClockReferenceSequenceCase>& paramInfo)
    {
        return paramInfo.param.name;
    }
);


// ============================================================================
// Longer Complete Clock Sequence
// ============================================================================

TEST_F(
    ClockPageReplacementPolicyTest,
    CompleteReplacementReferenceSequenceProducesExpectedVictims
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    /*
     * First selection:
     *
     * F0 = 1 -> clear
     * F1 = 1 -> clear
     * F2 = 1 -> clear
     * F0 = 0 -> victim
     *
     * Hand -> F1
     */
    auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    /*
     * Remove F0 and load P3 into the same frame.
     *
     * The hand is still logically at F1.
     */
    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage3,
        kFrame0
    );

    /*
     * F1 is clear, so it is selected.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    /*
     * Remove F1 and reuse it for P0.
     */
    policy_.pageRemoved(
        kPage1,
        kFrame1
    );

    policy_.pageLoaded(
        kPage0,
        kFrame1
    );

    /*
     * The hand advances after F1 selection to the next entry.
     *
     * The newly loaded F1 has reference bit set, so when encountered
     * it receives a second chance.
     */
    victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    /*
     * At this point the exact next victim is determined by the
     * circular hand and the newly loaded reference bit.
     *
     * The hand after selecting F1 points to F2, which is clear.
     */
    EXPECT_EQ(*victim, kFrame2);
}

} // namespace emmus::test