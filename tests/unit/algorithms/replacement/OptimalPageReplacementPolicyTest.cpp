#include <chrono>
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/OptimalPageReplacementPolicy.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"

namespace emmus::test
{

class OptimalPageReplacementPolicyTest
    : public PageReplacementAlgorithmFixture
{
protected:

    using Policy =
        emmus::algorithms::replacement::OptimalPageReplacementPolicy;

    using PageId =
        emmus::memory::identifiers::PageId;

    using FrameId =
        emmus::memory::identifiers::FrameId;

    Policy policy_;
};


// ============================================================================
// Empty State
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    EmptyPolicyReturnsNoVictim
)
{
    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
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
    OptimalPageReplacementPolicyTest,
    SingleLoadedPageIsSelectedWhenNoFutureUseExists
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence(
        {
            kPage1,
            kPage2
        }
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
    OptimalPageReplacementPolicyTest,
    SingleLoadedPageIsSelectedWhenFutureUseExists
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence(
        {
            kPage0
        }
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
// Reference Sequence
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    ReferenceSequenceCanBeConfigured
)
{
    const Policy::ReferenceSequence sequence{
        kPage0,
        kPage1,
        kPage2,
        kPage3
    };

    policy_.setReferenceSequence(sequence);

    EXPECT_EQ(
        policy_.referenceSequence(),
        sequence
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    ReplacingReferenceSequenceResetsReferencePosition
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence(
        {
            kPage0,
            kPage1
        }
    );

    policy_.pageAccessed(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence(
        {
            kPage1,
            kPage0
        }
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    /*
     * After resetting the reference sequence, both pages are evaluated
     * from the beginning of the new workload.
     *
     * P0 is used second and is therefore farther in the future.
     */
    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


// ============================================================================
// Page Loading
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    LoadedPagesAreTracked
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence(
        {
            kPage1
        }
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    /*
     * P0 has no future use, so it is the optimal victim.
     */
    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    DuplicatePageLoadDoesNotCreateDuplicateEntry
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence(
        {}
    );

    const auto firstVictim =
        policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());

    EXPECT_EQ(
        *firstVictim,
        kFrame0
    );

    policy_.pageRemoved(
        kPage0,
        kFrame0
    );

    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    ConflictingPageLoadDoesNotReplaceExistingMapping
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage0,
        kFrame1
    );

    policy_.setReferenceSequence({});

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    ConflictingFrameLoadDoesNotReplaceExistingMapping
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame0
    );

    policy_.setReferenceSequence({});

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


// ============================================================================
// Optimal Future-Use Selection
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    SelectsPageWhoseNextUseIsFarthestInFuture
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.pageLoaded(
        kPage2,
        kFrame2
    );

    /*
     * Future sequence:
     *
     * P1 -> P0 -> P2
     *
     * P2 has the farthest next use and must be selected.
     */
    policy_.setReferenceSequence(
        {
            kPage1,
            kPage0,
            kPage2
        }
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame2
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    SelectsPageWithNoFutureUseFirst
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.pageLoaded(
        kPage2,
        kFrame2
    );

    /*
     * P1 and P2 are referenced again.
     * P0 is never referenced again.
     */
    policy_.setReferenceSequence(
        {
            kPage1,
            kPage2
        }
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
    OptimalPageReplacementPolicyTest,
    RepeatedFutureReferencesUseTheNextOccurrence
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence(
        {
            kPage0,
            kPage1,
            kPage0,
            kPage1,
            kPage0
        }
    );

    /*
     * P0 next occurs at position 0.
     * P1 next occurs at position 1.
     *
     * Therefore P1 is farther in the future.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame1
    );
}


// ============================================================================
// Access Processing
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    AccessAdvancesFutureReferencePosition
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence(
        {
            kPage0,
            kPage1,
            kPage0
        }
    );

    /*
     * Consume P0 at the beginning of the workload.
     */
    policy_.pageAccessed(
        kPage0,
        kFrame0
    );

    /*
     * Remaining future:
     *
     * P1 -> P0
     *
     * P0 is now farther in the future.
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
    OptimalPageReplacementPolicyTest,
    UnknownPageAccessStillAdvancesReferenceSequence
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence(
        {
            kPage2,
            kPage0,
            kPage1
        }
    );

    /*
     * P2 is not resident, but its access still represents a real
     * workload position and must advance the Optimal reference cursor.
     */
    policy_.pageAccessed(
        kPage2,
        kFrame2
    );

    /*
     * Remaining future:
     *
     * P0 -> P1
     *
     * P1 is farther away and is therefore selected.
     */
    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame1
    );
}


// ============================================================================
// Deterministic Ties
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    EqualFutureUseDistanceUsesResidencyOrder
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence(
        {
            kPage0,
            kPage1
        }
    );

    /*
     * Both candidates have finite future-use positions.
     *
     * The implementation uses residency order as the deterministic
     * tie-breaker when the future-use distances are equivalent.
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
    OptimalPageReplacementPolicyTest,
    MultipleNeverReferencedPagesUseResidencyOrder
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.pageLoaded(
        kPage2,
        kFrame2
    );

    policy_.setReferenceSequence({});

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}


// ============================================================================
// Page Removal / Replacement
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    RemovingPageMakesItUnavailableForVictimSelection
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence({});

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
    OptimalPageReplacementPolicyTest,
    RemovingUnknownPageIsIgnored
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence({});

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
    OptimalPageReplacementPolicyTest,
    RemovingMismatchedFrameIsIgnored
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.setReferenceSequence({});

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


TEST_F(
    OptimalPageReplacementPolicyTest,
    VictimRemainsTrackedUntilRemoved
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence({});

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


// ============================================================================
// Replacement Statistics
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    SuccessfulVictimSelectionRecordsReplacement
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence({});

    EXPECT_EQ(
        policy_.statistics().replacementCount(),
        0U
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
    OptimalPageReplacementPolicyTest,
    RepeatedVictimSelectionsRecordEachSelection
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence({});

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
    OptimalPageReplacementPolicyTest,
    OptimalDoesNotRecordDirtyEvictions
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence({});

    ASSERT_TRUE(
        policy_.chooseVictim().has_value()
    );

    EXPECT_EQ(
        policy_.statistics().dirtyEvictionCount(),
        0U
    );
}


// ============================================================================
// Reset
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    ResetClearsTrackedPages
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence({});

    policy_.reset();

    EXPECT_FALSE(
        policy_.chooseVictim().has_value()
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    ResetClearsReferenceSequence
)
{
    policy_.setReferenceSequence(
        {
            kPage0,
            kPage1,
            kPage2
        }
    );

    policy_.reset();

    EXPECT_TRUE(
        policy_.referenceSequence().empty()
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    ResetClearsStatistics
)
{
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.setReferenceSequence({});

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
// Known Optimal Reference Sequences
// ============================================================================

TEST_F(
    OptimalPageReplacementPolicyTest,
    KnownOptimalSequenceSelectsFarthestFuturePage
)
{
    /*
     * Classic-style future sequence:
     *
     * Resident: P0, P1, P2
     *
     * Future:
     * P0, P1, P0, P2
     *
     * Next use:
     * P0 -> position 0
     * P1 -> position 1
     * P2 -> position 3
     *
     * P2 is the optimal victim.
     */
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.pageLoaded(
        kPage2,
        kFrame2
    );

    policy_.setReferenceSequence(
        {
            kPage0,
            kPage1,
            kPage0,
            kPage2
        }
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame2
    );
}


TEST_F(
    OptimalPageReplacementPolicyTest,
    KnownOptimalSequencePrefersNeverUsedPage
)
{
    /*
     * Resident: P0, P1, P2
     *
     * Future:
     * P1, P2, P1
     *
     * P0 never occurs again and is therefore the optimal victim.
     */
    policy_.pageLoaded(
        kPage0,
        kFrame0
    );

    policy_.pageLoaded(
        kPage1,
        kFrame1
    );

    policy_.pageLoaded(
        kPage2,
        kFrame2
    );

    policy_.setReferenceSequence(
        {
            kPage1,
            kPage2,
            kPage1
        }
    );

    const auto victim =
        policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        kFrame0
    );
}

} // namespace emmus::test