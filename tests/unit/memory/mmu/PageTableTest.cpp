#include <cstdint>
#include <limits>
#include <optional>

#include <gtest/gtest.h>

#include "fixtures/TestFixture.hpp"

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/mmu/PageTable.hpp"

namespace
{

using emmus::memory::identifiers::FrameId;
using emmus::memory::identifiers::PageId;
using emmus::memory::mmu::PageTable;


/**
 * @brief Unit-test fixture for the PageTable component.
 *
 * Uses the project's common TestFixture while retaining the strongly typed
 * production PageId and FrameId identifiers required by PageTable.
 */
class PageTableTest : public emmus::test::TestFixture
{
protected:

    using PageIdentifier = PageId;
    using FrameIdentifier = FrameId;

    // -----------------------------------------------------------------------
    // Page IDs
    // -----------------------------------------------------------------------

    static constexpr PageIdentifier kPage0{0};
    static constexpr PageIdentifier kPage1{1};
    static constexpr PageIdentifier kPage2{2};
    static constexpr PageIdentifier kPage3{3};

    // -----------------------------------------------------------------------
    // Frame IDs
    // -----------------------------------------------------------------------

    static constexpr FrameIdentifier kFrame0{0};
    static constexpr FrameIdentifier kFrame1{1};
    static constexpr FrameIdentifier kFrame2{2};
    static constexpr FrameIdentifier kFrame3{3};

    // -----------------------------------------------------------------------
    // System Under Test
    // -----------------------------------------------------------------------

    PageTable pageTable;
};

} // namespace


// ============================================================================
// Construction
// ============================================================================

TEST_F(
    PageTableTest,
    DefaultConstructionCreatesEmptyPageTable
)
{
    EXPECT_TRUE(pageTable.empty());
    EXPECT_EQ(pageTable.size(), 0U);
}


// ============================================================================
// Adding Mappings
// ============================================================================

TEST_F(
    PageTableTest,
    MapAddsNewPageToFrameMapping
)
{
    EXPECT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_TRUE(pageTable.isMapped(kPage1));
    EXPECT_EQ(pageTable.size(), 1U);
}


TEST_F(
    PageTableTest,
    MapStoresCorrectFrameForPage
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    const auto result = pageTable.lookup(kPage1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame1);
}


TEST_F(
    PageTableTest,
    MultiplePagesCanBeMappedToDifferentFrames
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    ASSERT_TRUE(
        pageTable.map(
            kPage2,
            kFrame2
        )
    );

    ASSERT_TRUE(
        pageTable.map(
            kPage3,
            kFrame3
        )
    );

    EXPECT_EQ(pageTable.size(), 3U);

    ASSERT_TRUE(pageTable.lookup(kPage1).has_value());
    ASSERT_TRUE(pageTable.lookup(kPage2).has_value());
    ASSERT_TRUE(pageTable.lookup(kPage3).has_value());

    EXPECT_EQ(*pageTable.lookup(kPage1), kFrame1);
    EXPECT_EQ(*pageTable.lookup(kPage2), kFrame2);
    EXPECT_EQ(*pageTable.lookup(kPage3), kFrame3);
}


// ============================================================================
// Retrieving Mappings
// ============================================================================

TEST_F(
    PageTableTest,
    LookupReturnsMappedFrame
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame2
        )
    );

    const auto result = pageTable.lookup(kPage1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame2);
}


TEST_F(
    PageTableTest,
    LookupReturnsNulloptForUnmappedPage
)
{
    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::nullopt
    );
}


TEST_F(
    PageTableTest,
    IsMappedReturnsFalseForUnmappedPage
)
{
    EXPECT_FALSE(
        pageTable.isMapped(kPage1)
    );
}


TEST_F(
    PageTableTest,
    IsMappedReturnsTrueForMappedPage
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_TRUE(
        pageTable.isMapped(kPage1)
    );
}


// ============================================================================
// Updating Mappings
// ============================================================================

TEST_F(
    PageTableTest,
    UpdateMappingChangesExistingFrame
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_TRUE(
        pageTable.updateMapping(
            kPage1,
            kFrame2
        )
    );

    const auto result = pageTable.lookup(kPage1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame2);
}


TEST_F(
    PageTableTest,
    UpdateMappingDoesNotChangeMappingCount
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_TRUE(
        pageTable.updateMapping(
            kPage1,
            kFrame2
        )
    );

    EXPECT_EQ(pageTable.size(), 1U);
}


TEST_F(
    PageTableTest,
    UpdateMappingFailsForUnmappedPage
)
{
    EXPECT_FALSE(
        pageTable.updateMapping(
            kPage1,
            kFrame1
        )
    );

    EXPECT_TRUE(pageTable.empty());
    EXPECT_FALSE(pageTable.isMapped(kPage1));
    EXPECT_EQ(pageTable.lookup(kPage1), std::nullopt);
}


TEST_F(
    PageTableTest,
    FailedUpdateDoesNotCreateMapping
)
{
    EXPECT_FALSE(
        pageTable.updateMapping(
            kPage2,
            kFrame2
        )
    );

    EXPECT_EQ(pageTable.size(), 0U);
    EXPECT_FALSE(pageTable.isMapped(kPage2));
}


// ============================================================================
// Duplicate Mappings
// ============================================================================

TEST_F(
    PageTableTest,
    DuplicateMappingIsRejected
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_FALSE(
        pageTable.map(
            kPage1,
            kFrame2
        )
    );
}


TEST_F(
    PageTableTest,
    DuplicateMappingDoesNotOverwriteExistingFrame
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_FALSE(
        pageTable.map(
            kPage1,
            kFrame2
        )
    );

    const auto result = pageTable.lookup(kPage1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame1);
}


TEST_F(
    PageTableTest,
    DuplicateMappingDoesNotIncreaseMappingCount
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_FALSE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_EQ(pageTable.size(), 1U);
}


// ============================================================================
// Removing Mappings
// ============================================================================

TEST_F(
    PageTableTest,
    UnmapRemovesExistingMapping
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_TRUE(
        pageTable.unmap(kPage1)
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage1)
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        std::nullopt
    );

    EXPECT_TRUE(pageTable.empty());
}


TEST_F(
    PageTableTest,
    UnmapDecreasesMappingCount
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    ASSERT_TRUE(
        pageTable.map(
            kPage2,
            kFrame2
        )
    );

    EXPECT_TRUE(
        pageTable.unmap(kPage1)
    );

    EXPECT_EQ(pageTable.size(), 1U);
    EXPECT_FALSE(pageTable.isMapped(kPage1));
    EXPECT_TRUE(pageTable.isMapped(kPage2));
}


TEST_F(
    PageTableTest,
    UnmapFailsForUnmappedPage
)
{
    EXPECT_FALSE(
        pageTable.unmap(kPage1)
    );

    EXPECT_TRUE(pageTable.empty());
}


TEST_F(
    PageTableTest,
    UnmappingOnePageDoesNotAffectOtherMappings
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    ASSERT_TRUE(
        pageTable.map(
            kPage2,
            kFrame2
        )
    );

    ASSERT_TRUE(
        pageTable.unmap(kPage1)
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage1)
    );

    const auto result = pageTable.lookup(kPage2);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame2);
}


// ============================================================================
// Re-mapping After Removal
// ============================================================================

TEST_F(
    PageTableTest,
    PageCanBeMappedAgainAfterBeingUnmapped
)
{
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    ASSERT_TRUE(
        pageTable.unmap(kPage1)
    );

    EXPECT_TRUE(
        pageTable.map(
            kPage1,
            kFrame2
        )
    );

    const auto result = pageTable.lookup(kPage1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame2);
}


// ============================================================================
// Identifier Edge Cases
// ============================================================================

TEST_F(
    PageTableTest,
    ZeroIdentifiersCanBeMapped
)
{
    EXPECT_TRUE(
        pageTable.map(
            kPage0,
            kFrame0
        )
    );

    const auto result = pageTable.lookup(kPage0);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, kFrame0);
}


TEST_F(
    PageTableTest,
    MaximumIdentifierValuesCanBeMapped
)
{
    const auto maximum =
        std::numeric_limits<std::uint64_t>::max();

    const PageIdentifier pageId{maximum};
    const FrameIdentifier frameId{maximum};

    EXPECT_TRUE(
        pageTable.map(
            pageId,
            frameId
        )
    );

    const auto result = pageTable.lookup(pageId);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, frameId);
}


// ============================================================================
// State Consistency
// ============================================================================

TEST_F(
    PageTableTest,
    EmptyReflectsCurrentMappingState
)
{
    EXPECT_TRUE(pageTable.empty());

    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_FALSE(pageTable.empty());

    ASSERT_TRUE(
        pageTable.unmap(kPage1)
    );

    EXPECT_TRUE(pageTable.empty());
}


TEST_F(
    PageTableTest,
    SizeTracksCurrentNumberOfMappings
)
{
    EXPECT_EQ(pageTable.size(), 0U);

    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            kFrame1
        )
    );

    EXPECT_EQ(pageTable.size(), 1U);

    ASSERT_TRUE(
        pageTable.map(
            kPage2,
            kFrame2
        )
    );

    EXPECT_EQ(pageTable.size(), 2U);

    ASSERT_TRUE(
        pageTable.unmap(kPage1)
    );

    EXPECT_EQ(pageTable.size(), 1U);

    ASSERT_TRUE(
        pageTable.unmap(kPage2)
    );

    EXPECT_EQ(pageTable.size(), 0U);
}