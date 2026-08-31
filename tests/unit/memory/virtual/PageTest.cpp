#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

#include <gtest/gtest.h>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/virtual/Page.hpp"

namespace
{

using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;
using emmus::memory::identifiers::ProcessId;
using emmus::memory::virtual_memory::Page;
using emmus::memory::virtual_memory::PageError;


// ============================================================================
// Compile-Time Interface Verification
// ============================================================================

static_assert(
    std::constructible_from<Page, PageId, ProcessId>
);

static_assert(
    std::copy_constructible<Page>
);

static_assert(
    std::is_nothrow_move_constructible_v<Page>
);

static_assert(
    std::same_as<
        decltype(std::declval<const Page&>().id()),
        PageId
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Page&>().processId()),
        ProcessId
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Page&>().isResident()),
        bool
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Page&>().mappedFrame()),
        const std::optional<FrameId>&
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Page&>().isDirty()),
        bool
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Page&>().isReferenced()),
        bool
    >
);

static_assert(
    std::same_as<
        decltype(
            std::declval<Page&>().mapToFrame(
                std::declval<FrameId>()
            )
        ),
        std::optional<PageError>
    >
);

static_assert(
    std::same_as<
        decltype(
            std::declval<Page&>().unmapFromFrame()
        ),
        std::optional<PageError>
    >
);

static_assert(
    !std::same_as<PageId, ProcessId>
);

static_assert(
    !std::same_as<PageId, FrameId>
);

static_assert(
    !std::same_as<ProcessId, FrameId>
);


// ============================================================================
// Construction and Identity
// ============================================================================

TEST(
    PageTest,
    ConstructionCreatesNonResidentPage
)
{
    const Page page{
        PageId{0},
        ProcessId{100}
    };

    EXPECT_FALSE(
        page.isResident()
    );

    EXPECT_FALSE(
        page.mappedFrame().has_value()
    );

    EXPECT_FALSE(
        page.isDirty()
    );

    EXPECT_FALSE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    ConstructionPreservesPageIdentity
)
{
    constexpr PageId expected{42};

    const Page page{
        expected,
        ProcessId{100}
    };

    EXPECT_EQ(
        page.id(),
        expected
    );
}


TEST(
    PageTest,
    ConstructionPreservesOwningProcessIdentity
)
{
    constexpr ProcessId expected{200};

    const Page page{
        PageId{7},
        expected
    };

    EXPECT_EQ(
        page.processId(),
        expected
    );
}


TEST(
    PageTest,
    ZeroIdentifiersAreValid
)
{
    const Page page{
        PageId{0},
        ProcessId{0}
    };

    EXPECT_EQ(
        page.id().value(),
        0U
    );

    EXPECT_EQ(
        page.processId().value(),
        0U
    );
}


TEST(
    PageTest,
    LargeIdentifiersArePreserved
)
{
    constexpr auto maximum =
        std::numeric_limits<std::uint64_t>::max();

    const Page page{
        PageId{maximum},
        ProcessId{maximum}
    };

    EXPECT_EQ(
        page.id().value(),
        maximum
    );

    EXPECT_EQ(
        page.processId().value(),
        maximum
    );
}


TEST(
    PageTest,
    SamePageIdInDifferentProcessesRemainsDistinctByContext
)
{
    const Page processOnePage{
        PageId{7},
        ProcessId{100}
    };

    const Page processTwoPage{
        PageId{7},
        ProcessId{200}
    };

    EXPECT_EQ(
        processOnePage.id(),
        processTwoPage.id()
    );

    EXPECT_NE(
        processOnePage.processId(),
        processTwoPage.processId()
    );
}


// ============================================================================
// Residency and Frame Mapping
// ============================================================================

TEST(
    PageTest,
    MappingNonResidentPageMakesItResident
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    const auto result =
        page.mapToFrame(FrameId{3});

    EXPECT_FALSE(
        result.has_value()
    );

    EXPECT_TRUE(
        page.isResident()
    );

    ASSERT_TRUE(
        page.mappedFrame().has_value()
    );

    EXPECT_EQ(
        page.mappedFrame().value(),
        FrameId{3}
    );
}


TEST(
    PageTest,
    MappingZeroFrameIdIsValid
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{0}).has_value()
    );

    ASSERT_TRUE(
        page.mappedFrame().has_value()
    );

    EXPECT_EQ(
        page.mappedFrame()->value(),
        0U
    );
}


TEST(
    PageTest,
    MappingSameFrameTwiceIsRejected
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    const auto result =
        page.mapToFrame(FrameId{3});

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        PageError::AlreadyResident
    );

    ASSERT_TRUE(
        page.mappedFrame().has_value()
    );

    EXPECT_EQ(
        page.mappedFrame()->value(),
        3U
    );
}


TEST(
    PageTest,
    MappingDifferentFrameWhileResidentIsRejected
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    const auto result =
        page.mapToFrame(FrameId{4});

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        PageError::AlreadyResident
    );

    ASSERT_TRUE(
        page.mappedFrame().has_value()
    );

    EXPECT_EQ(
        page.mappedFrame()->value(),
        3U
    );
}


// ============================================================================
// Unmapping
// ============================================================================

TEST(
    PageTest,
    UnmappingResidentPageMakesItNonResident
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    const auto result =
        page.unmapFromFrame();

    EXPECT_FALSE(
        result.has_value()
    );

    EXPECT_FALSE(
        page.isResident()
    );

    EXPECT_FALSE(
        page.mappedFrame().has_value()
    );
}


TEST(
    PageTest,
    UnmappingNonResidentPageIsRejected
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    const auto result =
        page.unmapFromFrame();

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        PageError::AlreadyNonResident
    );

    EXPECT_FALSE(
        page.isResident()
    );

    EXPECT_FALSE(
        page.mappedFrame().has_value()
    );
}


TEST(
    PageTest,
    RepeatedUnmappingIsRejected
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    ASSERT_FALSE(
        page.unmapFromFrame().has_value()
    );

    const auto result =
        page.unmapFromFrame();

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        PageError::AlreadyNonResident
    );

    EXPECT_FALSE(
        page.isResident()
    );
}


TEST(
    PageTest,
    NonResidentPageCanBeMappedAgainAfterUnmapping
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    ASSERT_FALSE(
        page.unmapFromFrame().has_value()
    );

    ASSERT_FALSE(
        page.mapToFrame(FrameId{7}).has_value()
    );

    EXPECT_TRUE(
        page.isResident()
    );

    ASSERT_TRUE(
        page.mappedFrame().has_value()
    );

    EXPECT_EQ(
        page.mappedFrame()->value(),
        7U
    );
}


// ============================================================================
// Dirty State
// ============================================================================

TEST(
    PageTest,
    NewPageIsClean
)
{
    const Page page{
        PageId{1},
        ProcessId{100}
    };

    EXPECT_FALSE(
        page.isDirty()
    );
}


TEST(
    PageTest,
    MarkingPageDirtySetsDirtyState
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    page.markDirty();

    EXPECT_TRUE(
        page.isDirty()
    );
}


TEST(
    PageTest,
    ClearingDirtyStateMakesPageClean
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    page.markDirty();
    page.clearDirty();

    EXPECT_FALSE(
        page.isDirty()
    );
}


TEST(
    PageTest,
    RepeatedDirtyOperationsAreIdempotent
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    page.markDirty();
    page.markDirty();

    EXPECT_TRUE(
        page.isDirty()
    );

    page.clearDirty();
    page.clearDirty();

    EXPECT_FALSE(
        page.isDirty()
    );
}


// ============================================================================
// Reference State
// ============================================================================

TEST(
    PageTest,
    NewPageIsUnreferenced
)
{
    const Page page{
        PageId{1},
        ProcessId{100}
    };

    EXPECT_FALSE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    MarkingPageReferencedSetsReferenceState
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    page.markReferenced();

    EXPECT_TRUE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    ClearingReferenceStateMakesPageUnreferenced
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    page.markReferenced();
    page.clearReferenced();

    EXPECT_FALSE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    RepeatedReferenceOperationsAreIdempotent
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    page.markReferenced();
    page.markReferenced();

    EXPECT_TRUE(
        page.isReferenced()
    );

    page.clearReferenced();
    page.clearReferenced();

    EXPECT_FALSE(
        page.isReferenced()
    );
}


// ============================================================================
// Mapping and State Interaction
// ============================================================================

TEST(
    PageTest,
    MappingDoesNotImplicitlyMarkPageDirty
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    EXPECT_FALSE(
        page.isDirty()
    );
}


TEST(
    PageTest,
    MappingDoesNotImplicitlyMarkPageReferenced
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    EXPECT_FALSE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    UnmappingResetsTransientResidentState
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    page.markDirty();
    page.markReferenced();

    ASSERT_TRUE(
        page.isDirty()
    );

    ASSERT_TRUE(
        page.isReferenced()
    );

    ASSERT_FALSE(
        page.unmapFromFrame().has_value()
    );

    EXPECT_FALSE(
        page.isResident()
    );

    EXPECT_FALSE(
        page.mappedFrame().has_value()
    );

    EXPECT_FALSE(
        page.isDirty()
    );

    EXPECT_FALSE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    FailedMappingDoesNotChangeExistingState
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_FALSE(
        page.mapToFrame(FrameId{3}).has_value()
    );

    page.markDirty();
    page.markReferenced();

    ASSERT_TRUE(
        page.mapToFrame(FrameId{7}).has_value()
    );

    EXPECT_TRUE(
        page.isResident()
    );

    ASSERT_TRUE(
        page.mappedFrame().has_value()
    );

    EXPECT_EQ(
        page.mappedFrame()->value(),
        3U
    );

    EXPECT_TRUE(
        page.isDirty()
    );

    EXPECT_TRUE(
        page.isReferenced()
    );
}


TEST(
    PageTest,
    FailedUnmappingDoesNotChangeExistingState
)
{
    Page page{
        PageId{1},
        ProcessId{100}
    };

    ASSERT_TRUE(
        page.unmapFromFrame().has_value()
    );

    EXPECT_FALSE(
        page.isResident()
    );

    EXPECT_FALSE(
        page.mappedFrame().has_value()
    );

    EXPECT_FALSE(
        page.isDirty()
    );

    EXPECT_FALSE(
        page.isReferenced()
    );
}


// ============================================================================
// Strong-Type Separation
// ============================================================================

TEST(
    PageTest,
    PageAndFrameIdentifiersAreDistinctTypes
)
{
    const PageId pageId{5};
    const FrameId frameId{5};

    EXPECT_EQ(
        pageId.value(),
        frameId.value()
    );

    EXPECT_FALSE(
        pageId == PageId{6}
    );

    EXPECT_FALSE(
        frameId == FrameId{6}
    );
}


TEST(
    PageTest,
    PageAndProcessIdentifiersAreDistinctTypes
)
{
    const PageId pageId{5};
    const ProcessId processId{5};

    EXPECT_EQ(
        pageId.value(),
        processId.value()
    );

    EXPECT_FALSE(
        pageId == PageId{6}
    );

    EXPECT_FALSE(
        processId == ProcessId{6}
    );
}

} // namespace