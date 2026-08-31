#include <gtest/gtest.h>

#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"
#include "fixtures/MemoryManagerFixture.hpp"

namespace emmus::test
{

class PhysicalMemoryManagerTest
    : public MemoryManagerFixture
{
protected:

    using ProductionPageId =
        emmus::memory::identifiers::PageId;

    using ProductionFrameId =
        emmus::memory::identifiers::FrameId;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;
};


// ============================================================================
// Initialization
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    InitializesConfiguredNumberOfFramesAsFree)
{
    constexpr std::size_t kFrameCount = 4;

    const PhysicalMemoryManager manager{kFrameCount};

    EXPECT_EQ(
        manager.capacity(),
        kFrameCount
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        kFrameCount
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        0U
    );

    EXPECT_TRUE(
        manager.hasFreeFrame()
    );


    EXPECT_TRUE(
        manager.isValidFrameId(
            ProductionFrameId{kFrame0}
        )
    );

    EXPECT_TRUE(
        manager.isValidFrameId(
            ProductionFrameId{kFrame1}
        )
    );

    EXPECT_TRUE(
        manager.isValidFrameId(
            ProductionFrameId{kFrame2}
        )
    );

    EXPECT_TRUE(
        manager.isValidFrameId(
            ProductionFrameId{kFrame3}
        )
    );


    EXPECT_TRUE(
        manager.isFrameFree(
            ProductionFrameId{kFrame0}
        )
    );

    EXPECT_TRUE(
        manager.isFrameFree(
            ProductionFrameId{kFrame1}
        )
    );

    EXPECT_TRUE(
        manager.isFrameFree(
            ProductionFrameId{kFrame2}
        )
    );

    EXPECT_TRUE(
        manager.isFrameFree(
            ProductionFrameId{kFrame3}
        )
    );
}


// ============================================================================
// Zero-Capacity Edge Case
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    SupportsZeroPhysicalFrames)
{
    PhysicalMemoryManager manager{0};

    EXPECT_EQ(
        manager.capacity(),
        0U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        0U
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        0U
    );

    EXPECT_FALSE(
        manager.hasFreeFrame()
    );


    const auto result =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );

    EXPECT_FALSE(
        result.has_value()
    );
}


// ============================================================================
// Successful Allocation
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    AllocatesFirstFreeFrame)
{
    PhysicalMemoryManager manager{4};

    const auto result =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );


    ASSERT_TRUE(
        result.has_value()
    );

    EXPECT_EQ(
        result.value(),
        ProductionFrameId{kFrame0}
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        1U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        3U
    );

    EXPECT_TRUE(
        manager.isPageMapped(
            ProductionPageId{kPage0}
        )
    );

    EXPECT_FALSE(
        manager.isFrameFree(
            ProductionFrameId{kFrame0}
        )
    );


    const auto mappedFrame =
        manager.frameForPage(
            ProductionPageId{kPage0}
        );

    ASSERT_TRUE(
        mappedFrame.has_value()
    );

    EXPECT_EQ(
        mappedFrame.value(),
        ProductionFrameId{kFrame0}
    );
}


// ============================================================================
// Multiple Allocations
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    MultipleAllocationsUseDistinctFrames)
{
    PhysicalMemoryManager manager{4};


    const auto frame0 =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );

    const auto frame1 =
        manager.allocateFrame(
            ProductionPageId{kPage1}
        );

    const auto frame2 =
        manager.allocateFrame(
            ProductionPageId{kPage2}
        );

    const auto frame3 =
        manager.allocateFrame(
            ProductionPageId{kPage3}
        );


    ASSERT_TRUE(frame0.has_value());
    ASSERT_TRUE(frame1.has_value());
    ASSERT_TRUE(frame2.has_value());
    ASSERT_TRUE(frame3.has_value());


    EXPECT_EQ(
        frame0.value(),
        ProductionFrameId{kFrame0}
    );

    EXPECT_EQ(
        frame1.value(),
        ProductionFrameId{kFrame1}
    );

    EXPECT_EQ(
        frame2.value(),
        ProductionFrameId{kFrame2}
    );

    EXPECT_EQ(
        frame3.value(),
        ProductionFrameId{kFrame3}
    );


    EXPECT_EQ(
        manager.allocatedFrameCount(),
        4U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        0U
    );

    EXPECT_FALSE(
        manager.hasFreeFrame()
    );
}


// ============================================================================
// Frame Exhaustion
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    AllocationFailsWhenAllFramesAreOccupied)
{
    PhysicalMemoryManager manager{4};


    ASSERT_TRUE(
        manager.allocateFrame(
            ProductionPageId{kPage0}
        ).has_value()
    );

    ASSERT_TRUE(
        manager.allocateFrame(
            ProductionPageId{kPage1}
        ).has_value()
    );

    ASSERT_TRUE(
        manager.allocateFrame(
            ProductionPageId{kPage2}
        ).has_value()
    );

    ASSERT_TRUE(
        manager.allocateFrame(
            ProductionPageId{kPage3}
        ).has_value()
    );


    const auto result =
        manager.allocateFrame(
            ProductionPageId{999}
        );


    EXPECT_FALSE(
        result.has_value()
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        4U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        0U
    );

    EXPECT_FALSE(
        manager.hasFreeFrame()
    );
}


// ============================================================================
// Release
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    ReleaseMakesFrameAvailableAgain)
{
    PhysicalMemoryManager manager{4};


    const auto allocatedFrame =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );

    ASSERT_TRUE(
        allocatedFrame.has_value()
    );


    EXPECT_TRUE(
        manager.releaseFrame(
            allocatedFrame.value()
        )
    );


    EXPECT_EQ(
        manager.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        4U
    );

    EXPECT_TRUE(
        manager.hasFreeFrame()
    );

    EXPECT_TRUE(
        manager.isFrameFree(
            ProductionFrameId{kFrame0}
        )
    );

    EXPECT_FALSE(
        manager.isPageMapped(
            ProductionPageId{kPage0}
        )
    );


    const auto mappedFrame =
        manager.frameForPage(
            ProductionPageId{kPage0}
        );

    EXPECT_FALSE(
        mappedFrame.has_value()
    );
}


// ============================================================================
// Reallocation
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    ReleasedFrameCanBeReallocated)
{
    PhysicalMemoryManager manager{2};


    const auto firstAllocation =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );

    const auto secondAllocation =
        manager.allocateFrame(
            ProductionPageId{kPage1}
        );


    ASSERT_TRUE(firstAllocation.has_value());
    ASSERT_TRUE(secondAllocation.has_value());


    EXPECT_TRUE(
        manager.releaseFrame(
            firstAllocation.value()
        )
    );


    const auto reallocation =
        manager.allocateFrame(
            ProductionPageId{kPage2}
        );


    ASSERT_TRUE(
        reallocation.has_value()
    );


    EXPECT_EQ(
        reallocation.value(),
        firstAllocation.value()
    );

    EXPECT_TRUE(
        manager.isPageMapped(
            ProductionPageId{kPage2}
        )
    );

    EXPECT_FALSE(
        manager.isPageMapped(
            ProductionPageId{kPage0}
        )
    );

    EXPECT_TRUE(
        manager.isPageMapped(
            ProductionPageId{kPage1}
        )
    );
}


// ============================================================================
// Duplicate Page Allocation
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    RejectsAllocationWhenPageIsAlreadyMapped)
{
    PhysicalMemoryManager manager{2};


    const auto firstAllocation =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );

    ASSERT_TRUE(
        firstAllocation.has_value()
    );


    const auto duplicateAllocation =
        manager.allocateFrame(
            ProductionPageId{kPage0}
        );


    EXPECT_FALSE(
        duplicateAllocation.has_value()
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        1U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        1U
    );


    const auto mappedFrame =
        manager.frameForPage(
            ProductionPageId{kPage0}
        );

    ASSERT_TRUE(
        mappedFrame.has_value()
    );

    EXPECT_EQ(
        mappedFrame.value(),
        firstAllocation.value()
    );
}


// ============================================================================
// Invalid Frame Identifier
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    InvalidFrameIdIsHandledSafely)
{
    const PhysicalMemoryManager manager{4};

    const ProductionFrameId invalidFrameId{999};


    EXPECT_FALSE(
        manager.isValidFrameId(invalidFrameId)
    );

    EXPECT_FALSE(
        manager.isFrameFree(invalidFrameId)
    );

    EXPECT_EQ(
        manager.frame(invalidFrameId),
        nullptr
    );
}


// ============================================================================
// Invalid Release
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    ReleasingInvalidFrameDoesNotChangeAllocationState)
{
    PhysicalMemoryManager manager{4};

    ProductionFrameId invalidFrameId{999};


    EXPECT_FALSE(
        manager.releaseFrame(invalidFrameId)
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        4U
    );
}


// ============================================================================
// Already-Free Frame
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    ReleasingAlreadyFreeFrameDoesNotChangeAllocationState)
{
    PhysicalMemoryManager manager{4};


    EXPECT_FALSE(
        manager.releaseFrame(
            ProductionFrameId{kFrame0}
        )
    );

    EXPECT_EQ(
        manager.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        manager.freeFrameCount(),
        4U
    );

    EXPECT_TRUE(
        manager.isFrameFree(
            ProductionFrameId{kFrame0}
        )
    );
}


// ============================================================================
// Page Lookup
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    UnmappedPageHasNoAssociatedFrame)
{
    const PhysicalMemoryManager manager{4};

    const ProductionPageId pageId{999};


    EXPECT_FALSE(
        manager.isPageMapped(pageId)
    );

    EXPECT_FALSE(
        manager.frameForPage(pageId).has_value()
    );
}


// ============================================================================
// Strong Identifier Type Verification
// ============================================================================

TEST_F(
    PhysicalMemoryManagerTest,
    UsesDistinctStronglyTypedPageAndFrameIdentifiers)
{
    static_assert(
        !std::is_same_v<
            ProductionPageId,
            ProductionFrameId
        >
    );

    static_assert(
        !std::is_convertible_v<
            ProductionPageId,
            ProductionFrameId
        >
    );

    static_assert(
        !std::is_convertible_v<
            ProductionFrameId,
            ProductionPageId
        >
    );
}

} // namespace emmus::test