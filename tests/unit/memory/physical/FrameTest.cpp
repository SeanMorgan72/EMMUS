#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

#include <gtest/gtest.h>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/physical/Frame.hpp"

namespace
{

using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;
using emmus::memory::physical::Frame;
using emmus::memory::physical::FrameError;


// ============================================================================
// Compile-Time Interface Verification
// ============================================================================

static_assert(
    std::constructible_from<Frame, FrameId>
);

static_assert(
    std::copy_constructible<Frame>
);

static_assert(
    std::is_nothrow_move_constructible_v<Frame>
);

static_assert(
    std::same_as<
        decltype(std::declval<const Frame&>().id()),
        FrameId
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Frame&>().isOccupied()),
        bool
    >
);

static_assert(
    std::same_as<
        decltype(std::declval<const Frame&>().mappedPage()),
        const std::optional<PageId>&
    >
);

static_assert(
    std::same_as<
        decltype(
            std::declval<Frame&>().mapPage(
                std::declval<PageId>()
            )
        ),
        std::optional<FrameError>
    >
);

static_assert(
    std::same_as<
        decltype(
            std::declval<Frame&>().unmapPage()
        ),
        std::optional<FrameError>
    >
);


// ============================================================================
// Construction and Identity
// ============================================================================

TEST(
    FrameTest,
    ConstructionCreatesFreeFrame
)
{
    const Frame frame{FrameId{0}};

    EXPECT_FALSE(
        frame.isOccupied()
    );

    EXPECT_FALSE(
        frame.mappedPage().has_value()
    );
}


TEST(
    FrameTest,
    ConstructionPreservesFrameIdentity
)
{
    constexpr FrameId expected{42};

    const Frame frame{expected};

    EXPECT_EQ(
        frame.id(),
        expected
    );
}


TEST(
    FrameTest,
    ZeroIsValidFrameIdentity
)
{
    const Frame frame{FrameId{0}};

    EXPECT_EQ(
        frame.id().value(),
        0U
    );
}


TEST(
    FrameTest,
    LargeFrameIdentityIsPreserved
)
{
    constexpr FrameId expected{
        std::numeric_limits<FrameId::ValueType>::max()
    };

    const Frame frame{expected};

    EXPECT_EQ(
        frame.id(),
        expected
    );
}


// ============================================================================
// Mapping
// ============================================================================

TEST(
    FrameTest,
    MappingPageOccupiesFreeFrame
)
{
    Frame frame{FrameId{1}};

    const auto result =
        frame.mapPage(PageId{7});

    EXPECT_FALSE(
        result.has_value()
    );

    EXPECT_TRUE(
        frame.isOccupied()
    );

    ASSERT_TRUE(
        frame.mappedPage().has_value()
    );

    EXPECT_EQ(
        frame.mappedPage().value(),
        PageId{7}
    );
}


TEST(
    FrameTest,
    MappingZeroPageIdIsValid
)
{
    Frame frame{FrameId{0}};

    const auto result =
        frame.mapPage(PageId{0});

    EXPECT_FALSE(
        result.has_value()
    );

    ASSERT_TRUE(
        frame.mappedPage().has_value()
    );

    EXPECT_EQ(
        frame.mappedPage()->value(),
        0U
    );
}


TEST(
    FrameTest,
    MappingSamePageTwiceIsRejected
)
{
    Frame frame{FrameId{1}};

    ASSERT_FALSE(
        frame.mapPage(PageId{7}).has_value()
    );

    const auto result =
        frame.mapPage(PageId{7});

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        FrameError::AlreadyOccupied
    );

    ASSERT_TRUE(
        frame.mappedPage().has_value()
    );

    EXPECT_EQ(
        frame.mappedPage()->value(),
        7U
    );
}


TEST(
    FrameTest,
    MappingDifferentPageIntoOccupiedFrameIsRejected
)
{
    Frame frame{FrameId{1}};

    ASSERT_FALSE(
        frame.mapPage(PageId{7}).has_value()
    );

    const auto result =
        frame.mapPage(PageId{8});

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        FrameError::AlreadyOccupied
    );

    ASSERT_TRUE(
        frame.mappedPage().has_value()
    );

    EXPECT_EQ(
        frame.mappedPage()->value(),
        7U
    );
}


// ============================================================================
// Unmapping
// ============================================================================

TEST(
    FrameTest,
    UnmappingPageFreesOccupiedFrame
)
{
    Frame frame{FrameId{1}};

    ASSERT_FALSE(
        frame.mapPage(PageId{7}).has_value()
    );

    const auto result =
        frame.unmapPage();

    EXPECT_FALSE(
        result.has_value()
    );

    EXPECT_FALSE(
        frame.isOccupied()
    );

    EXPECT_FALSE(
        frame.mappedPage().has_value()
    );
}


TEST(
    FrameTest,
    UnmappingFreeFrameIsRejected
)
{
    Frame frame{FrameId{1}};

    const auto result =
        frame.unmapPage();

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        FrameError::AlreadyFree
    );

    EXPECT_FALSE(
        frame.isOccupied()
    );

    EXPECT_FALSE(
        frame.mappedPage().has_value()
    );
}


TEST(
    FrameTest,
    RepeatedUnmappingIsRejected
)
{
    Frame frame{FrameId{1}};

    ASSERT_FALSE(
        frame.mapPage(PageId{7}).has_value()
    );

    ASSERT_FALSE(
        frame.unmapPage().has_value()
    );

    const auto result =
        frame.unmapPage();

    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        FrameError::AlreadyFree
    );

    EXPECT_FALSE(
        frame.isOccupied()
    );
}


// ============================================================================
// State Reuse and Preservation
// ============================================================================

TEST(
    FrameTest,
    FreeFrameCanBeReusedAfterUnmapping
)
{
    Frame frame{FrameId{2}};

    ASSERT_FALSE(
        frame.mapPage(PageId{10}).has_value()
    );

    ASSERT_FALSE(
        frame.unmapPage().has_value()
    );

    ASSERT_FALSE(
        frame.mapPage(PageId{20}).has_value()
    );

    EXPECT_TRUE(
        frame.isOccupied()
    );

    ASSERT_TRUE(
        frame.mappedPage().has_value()
    );

    EXPECT_EQ(
        frame.mappedPage()->value(),
        20U
    );
}


TEST(
    FrameTest,
    FailedMappingDoesNotChangeState
)
{
    Frame frame{FrameId{2}};

    ASSERT_FALSE(
        frame.mapPage(PageId{10}).has_value()
    );

    ASSERT_TRUE(
        frame.mapPage(PageId{20}).has_value()
    );

    EXPECT_TRUE(
        frame.isOccupied()
    );

    ASSERT_TRUE(
        frame.mappedPage().has_value()
    );

    EXPECT_EQ(
        frame.mappedPage()->value(),
        10U
    );
}


TEST(
    FrameTest,
    FailedUnmappingDoesNotChangeState
)
{
    Frame frame{FrameId{2}};

    ASSERT_TRUE(
        frame.unmapPage().has_value()
    );

    EXPECT_FALSE(
        frame.isOccupied()
    );

    EXPECT_FALSE(
        frame.mappedPage().has_value()
    );
}


// ============================================================================
// Strong-Type Separation
// ============================================================================

static_assert(
    !std::same_as<FrameId, PageId>
);


TEST(
    FrameTest,
    FrameIdentityAndPageIdentityAreDistinctTypes
)
{
    const FrameId frameId{5};
    const PageId pageId{5};

    EXPECT_EQ(
        frameId.value(),
        pageId.value()
    );

    EXPECT_FALSE(
        frameId == FrameId{6}
    );

    EXPECT_FALSE(
        pageId == PageId{6}
    );
}

} // namespace
