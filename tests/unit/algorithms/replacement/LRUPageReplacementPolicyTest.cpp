#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/LRUPageReplacementPolicy.hpp"
#include "emmus/statistics/PageReplacementStatistics.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"

namespace emmus::test
{

class LRUPageReplacementPolicyTest : public PageReplacementAlgorithmFixture
{
protected:
    using Policy = emmus::algorithms::replacement::LRUPageReplacementPolicy;
    using Statistics = emmus::statistics::PageReplacementStatistics;

    Policy policy_;
};

// ============================================================================
// Empty State
// ============================================================================

TEST_F(LRUPageReplacementPolicyTest, EmptyPolicyReturnsNoVictim)
{
    EXPECT_FALSE(policy_.chooseVictim().has_value());
}

TEST_F(LRUPageReplacementPolicyTest, EmptyPolicyDoesNotRecordReplacement)
{
    const auto victim = policy_.chooseVictim();

    EXPECT_FALSE(victim.has_value());
    EXPECT_EQ(policy_.statistics().replacementCount(), 0U);
    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        std::chrono::nanoseconds::zero()
    );
}

// ============================================================================
// Single Page / Frame
// ============================================================================

TEST_F(LRUPageReplacementPolicyTest, SingleLoadedPageIsSelectedAsVictim)
{
    policy_.pageLoaded(kPage0, kFrame0);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(LRUPageReplacementPolicyTest, RepeatedAccessToSinglePageKeepsItAsVictim)
{
    policy_.pageLoaded(kPage0, kFrame0);

    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage0, kFrame0);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

// ============================================================================
// Page Loading
// ============================================================================

TEST_F(LRUPageReplacementPolicyTest, LoadedPagesAreTracked)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    DuplicatePageLoadDoesNotCreateDuplicateEntry
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame0);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    ConflictingPageLoadDoesNotReplaceExistingMapping
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage0, kFrame1);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

// ============================================================================
// Access Ordering
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    AccessingOldestPageMovesItToMostRecentlyUsed
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage0, kFrame0);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    AccessingMiddlePageMovesItToMostRecentlyUsed
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage1, kFrame1);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    AccessingNewestPageKeepsOldestPageAsVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage2, kFrame2);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    RepeatedAccessUpdatesRecencyEachTime
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage1, kFrame1);
    policy_.pageAccessed(kPage0, kFrame0);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    DeterministicAccessSequenceProducesExpectedVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);
    policy_.pageLoaded(kPage3, kFrame3);

    policy_.pageAccessed(kPage2, kFrame2);
    policy_.pageAccessed(kPage0, kFrame0);
    policy_.pageAccessed(kPage3, kFrame3);
    policy_.pageAccessed(kPage2, kFrame2);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);
}

// ============================================================================
// Invalid Access Notifications
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    AccessToUnknownPageIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageAccessed(kPage2, kFrame2);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    AccessWithMismatchedFrameIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageAccessed(kPage0, kFrame1);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

// ============================================================================
// Victim Selection Persistence
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    ChooseVictimDoesNotRemovePage
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    const auto firstVictim = policy_.chooseVictim();
    const auto secondVictim = policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());
    ASSERT_TRUE(secondVictim.has_value());

    EXPECT_EQ(*firstVictim, kFrame0);
    EXPECT_EQ(*secondVictim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    VictimIsRemovedOnlyAfterPageRemovedNotification
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    const auto firstVictim = policy_.chooseVictim();

    ASSERT_TRUE(firstVictim.has_value());
    EXPECT_EQ(*firstVictim, kFrame0);

    policy_.pageRemoved(kPage0, kFrame0);

    const auto secondVictim = policy_.chooseVictim();

    ASSERT_TRUE(secondVictim.has_value());
    EXPECT_EQ(*secondVictim, kFrame1);
}

// ============================================================================
// Page Removal
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingLeastRecentlyUsedPageUpdatesVictim
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageRemoved(kPage0, kFrame0);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingMostRecentlyUsedPageLeavesOlderPagesTracked
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageRemoved(kPage2, kFrame2);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingUnknownPageIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageRemoved(kPage2, kFrame2);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    RemovingPageWithMismatchedFrameIsIgnored
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    policy_.pageRemoved(kPage0, kFrame1);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);
}

// ============================================================================
// Replacement Cycles
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    ReplacementCycleMaintainsCorrectLruOrder
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage0, kFrame0);

    auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage3, kFrame1);

    victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    MultipleReplacementCyclesProduceExpectedVictims
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.pageAccessed(kPage0, kFrame0);

    auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage3, kFrame1);

    policy_.pageAccessed(kPage2, kFrame2);

    victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame0);

    policy_.pageAccessed(kPage3, kFrame1);

    victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);
}

// ============================================================================
// Reset
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    ResetClearsTrackedPages
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    policy_.reset();

    EXPECT_FALSE(policy_.chooseVictim().has_value());
}

TEST_F(
    LRUPageReplacementPolicyTest,
    ResetClearsStatistics
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_TRUE(policy_.chooseVictim().has_value());

    EXPECT_GT(policy_.statistics().replacementCount(), 0U);

    policy_.reset();

    EXPECT_EQ(policy_.statistics().replacementCount(), 0U);
    EXPECT_EQ(policy_.statistics().dirtyEvictionCount(), 0U);
    EXPECT_EQ(
        policy_.statistics().totalExecutionTime(),
        std::chrono::nanoseconds::zero()
    );
}

// ============================================================================
// Statistics
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    SuccessfulVictimSelectionRecordsReplacement
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    EXPECT_EQ(policy_.statistics().replacementCount(), 0U);

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(policy_.statistics().replacementCount(), 1U);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    RepeatedVictimSelectionRecordsEachSelection
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_TRUE(policy_.chooseVictim().has_value());
    ASSERT_TRUE(policy_.chooseVictim().has_value());
    ASSERT_TRUE(policy_.chooseVictim().has_value());

    EXPECT_EQ(policy_.statistics().replacementCount(), 3U);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    EmptyVictimSelectionDoesNotRecordReplacement
)
{
    EXPECT_FALSE(policy_.chooseVictim().has_value());

    EXPECT_EQ(policy_.statistics().replacementCount(), 0U);
}

TEST_F(
    LRUPageReplacementPolicyTest,
    LRUDoesNotRecordDirtyEvictions
)
{
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);

    ASSERT_TRUE(policy_.chooseVictim().has_value());

    EXPECT_EQ(policy_.statistics().dirtyEvictionCount(), 0U);
}

// ============================================================================
// US-502: LRU Reference-Sequence Testing
// ============================================================================

struct LRUReferenceSequenceCase
{
    std::string name;

    std::vector<emmus::memory::identifiers::PageId> initialPages;
    std::vector<emmus::memory::identifiers::FrameId> initialFrames;

    std::vector<emmus::memory::identifiers::PageId> accessPages;
    std::vector<emmus::memory::identifiers::FrameId> accessFrames;

    emmus::memory::identifiers::FrameId expectedVictim;
};

class LRUReferenceSequenceTest
    : public LRUPageReplacementPolicyTest,
      public ::testing::WithParamInterface<LRUReferenceSequenceCase>
{
};

TEST_P(
    LRUReferenceSequenceTest,
    ReferenceSequenceProducesExpectedVictim
)
{
    const auto& testCase = GetParam();

    ASSERT_EQ(
        testCase.initialPages.size(),
        testCase.initialFrames.size()
    );

    ASSERT_EQ(
        testCase.accessPages.size(),
        testCase.accessFrames.size()
    );

    for (std::size_t i = 0; i < testCase.initialPages.size(); ++i)
    {
        policy_.pageLoaded(
            testCase.initialPages[i],
            testCase.initialFrames[i]
        );
    }

    for (std::size_t i = 0; i < testCase.accessPages.size(); ++i)
    {
        policy_.pageAccessed(
            testCase.accessPages[i],
            testCase.accessFrames[i]
        );
    }

    const auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());

    EXPECT_EQ(
        *victim,
        testCase.expectedVictim
    );
}

INSTANTIATE_TEST_SUITE_P(
    DeterministicReferenceSequences,
    LRUReferenceSequenceTest,
    ::testing::Values(

        // --------------------------------------------------------------------
        // Initial load order:
        // LRU -> MRU: F0, F1, F2
        // Expected victim: F0
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
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

        // --------------------------------------------------------------------
        // Access oldest page once:
        // Initial: F0, F1, F2
        // Access F0
        // Final: F1, F2, F0
        // Expected victim: F1
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "AccessOldestOnce",
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
            emmus::memory::identifiers::FrameId{1}
        },

        // --------------------------------------------------------------------
        // Access middle page once:
        // Initial: F0, F1, F2
        // Access F1
        // Final: F0, F2, F1
        // Expected victim: F0
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "AccessMiddleOnce",
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
                emmus::memory::identifiers::PageId{1}
            },
            {
                emmus::memory::identifiers::FrameId{1}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        // --------------------------------------------------------------------
        // Access newest page once:
        // Initial: F0, F1, F2
        // Access F2
        // Final: F0, F1, F2
        // Expected victim: F0
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "AccessNewestOnce",
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
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{2}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        // --------------------------------------------------------------------
        // Repeated oldest references:
        // Access sequence: F0, F0, F0
        // Final: F1, F2, F0
        // Expected victim: F1
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "RepeatedOldestReferences",
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
            emmus::memory::identifiers::FrameId{1}
        },

        // --------------------------------------------------------------------
        // Repeated middle references:
        // Access sequence: F1, F1, F1
        // Final: F0, F2, F1
        // Expected victim: F0
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "RepeatedMiddleReferences",
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
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{1}
            },
            {
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{1}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        // --------------------------------------------------------------------
        // Repeated newest references:
        // Access sequence: F2, F2, F2
        // Final: F0, F1, F2
        // Expected victim: F0
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "RepeatedNewestReferences",
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
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{2}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        // --------------------------------------------------------------------
        // Mixed three-page working set:
        // Initial: F0, F1, F2
        // Access: F0, F2, F1
        // Final: F0, F2, F1
        // Expected victim: F0
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "MixedThreePageWorkingSet",
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
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{1}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{1}
            },
            emmus::memory::identifiers::FrameId{0}
        },

        // --------------------------------------------------------------------
        // Mixed four-page working set:
        // Final LRU order: F1, F0, F3, F2
        // Expected victim: F1
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "MixedFourPageWorkingSet",
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
                emmus::memory::identifiers::PageId{2}
            },
            {
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{3},
                emmus::memory::identifiers::FrameId{2}
            },
            emmus::memory::identifiers::FrameId{1}
        },

        // --------------------------------------------------------------------
        // Repeated mixed references:
        // Access sequence: F0, F1, F0, F2, F1, F0
        // Final LRU order: F2, F1, F0
        // Expected victim: F2
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "RepeatedMixedReferences",
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
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{0}
            },
            {
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{0}
            },
            emmus::memory::identifiers::FrameId{2}
        },

        // --------------------------------------------------------------------
        // Longer four-page working-set pattern:
        // Final LRU order: F3, F2, F1, F0
        // Expected victim: F3
        // --------------------------------------------------------------------
        LRUReferenceSequenceCase{
            "LongerWorkingSetReferencePattern",
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
                emmus::memory::identifiers::PageId{3},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{0},
                emmus::memory::identifiers::PageId{3},
                emmus::memory::identifiers::PageId{2},
                emmus::memory::identifiers::PageId{1},
                emmus::memory::identifiers::PageId{0}
            },
            {
                emmus::memory::identifiers::FrameId{3},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{0},
                emmus::memory::identifiers::FrameId{3},
                emmus::memory::identifiers::FrameId{2},
                emmus::memory::identifiers::FrameId{1},
                emmus::memory::identifiers::FrameId{0}
            },
            emmus::memory::identifiers::FrameId{3}
        }
    ),
    [](const ::testing::TestParamInfo<LRUReferenceSequenceCase>& paramInfo)
    {
        return paramInfo.param.name;
    }
);

// ============================================================================
// US-502: Complete Replacement Reference Sequence
// ============================================================================

TEST_F(
    LRUPageReplacementPolicyTest,
    CompleteReplacementReferenceSequenceProducesExpectedVictims
)
{
    // Initial state:
    // LRU -> MRU: F0, F1, F2
    policy_.pageLoaded(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame1);
    policy_.pageLoaded(kPage2, kFrame2);

    // Access P0:
    // LRU -> MRU: F1, F2, F0
    policy_.pageAccessed(kPage0, kFrame0);

    auto victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame1);

    // Replace P1 in F1 with P3.
    policy_.pageRemoved(kPage1, kFrame1);
    policy_.pageLoaded(kPage3, kFrame1);

    // Current:
    // LRU -> MRU: F2, F0, F1
    victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);

    // Access P2:
    // LRU -> MRU: F0, F1, F2
    policy_.pageAccessed(kPage2, kFrame2);

    victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame0);

    // Replace P0 in F0 with P1.
    policy_.pageRemoved(kPage0, kFrame0);
    policy_.pageLoaded(kPage1, kFrame0);

    // Current:
    // LRU -> MRU: F1, F2, F0
    policy_.pageAccessed(kPage3, kFrame1);

    // Final:
    // LRU -> MRU: F2, F0, F1
    victim = policy_.chooseVictim();

    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, kFrame2);
}

} // namespace emmus::test