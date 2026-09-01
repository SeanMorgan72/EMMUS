#include <gtest/gtest.h>

#include "fixtures/MemoryManagerFixture.hpp"

#include "emmus/memory/mmu/PageTable.hpp"
#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"
#include "emmus/memory/physical/PhysicalMemoryManager.hpp"

namespace emmus::test
{

/**
 * @brief Integration tests for PageTable and PhysicalMemoryManager.
 *
 * Verifies US-203:
 *
 * - Physical frame allocation is coordinated with page-table mapping.
 * - PageTable entries are created only after successful frame allocation.
 * - Page-to-frame relationships remain consistent.
 * - Unmapping releases the corresponding physical frame.
 * - Frame release removes the corresponding page-table mapping.
 * - Allocation and mapping failures do not leave partial state.
 * - Conflicting mappings are rejected safely.
 *
 * Each test constructs fresh production objects to preserve test isolation.
 */
class PageTablePhysicalMemoryIntegrationTest
    : public MemoryManagerFixture
{
protected:

    using PageTable =
        emmus::memory::mmu::PageTable;

    using PageTablePhysicalMemoryIntegration =
        emmus::memory::mmu::PageTablePhysicalMemoryIntegration;

    using PhysicalMemoryManager =
        emmus::memory::physical::PhysicalMemoryManager;

    using Frame =
        emmus::memory::physical::Frame;


    PageTable pageTable;

    PhysicalMemoryManager physicalMemory{
        4
    };

    PageTablePhysicalMemoryIntegration integration{
        pageTable,
        physicalMemory
    };
};


// ============================================================================
// US-203: Successful Page-to-Frame Mapping
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_MapPageAllocatesFrameAndCreatesMapping
)
{
    // Arrange
    EXPECT_TRUE(
        pageTable.empty()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    // Act
    const auto frameId =
        integration.mapPage(kPage0);

    // Assert
    ASSERT_TRUE(
        frameId.has_value()
    );

    EXPECT_EQ(
        frameId.value(),
        kFrame0
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        frameId
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        frameId
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        1U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        3U
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage0)
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Correct Frame Ownership
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_MappedPageReferencesCorrectAllocatedFrame
)
{
    // Arrange
    const auto frameId =
        integration.mapPage(kPage0);

    ASSERT_TRUE(
        frameId.has_value()
    );

    // Act
    const Frame* frame =
        physicalMemory.frame(frameId.value());

    // Assert
    ASSERT_NE(
        frame,
        nullptr
    );

    EXPECT_TRUE(
        frame->isOccupied()
    );

    ASSERT_TRUE(
        frame->mappedPage().has_value()
    );

    EXPECT_EQ(
        frame->mappedPage().value(),
        kPage0
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        frameId
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        frameId
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage0)
    );
}


// ============================================================================
// US-203: Multiple Page Mappings
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_MultiplePagesReceiveDistinctFrames
)
{
    // Arrange / Act
    const auto frame0 =
        integration.mapPage(kPage0);

    const auto frame1 =
        integration.mapPage(kPage1);

    const auto frame2 =
        integration.mapPage(kPage2);

    // Assert
    ASSERT_TRUE(
        frame0.has_value()
    );

    ASSERT_TRUE(
        frame1.has_value()
    );

    ASSERT_TRUE(
        frame2.has_value()
    );

    EXPECT_EQ(
        frame0.value(),
        kFrame0
    );

    EXPECT_EQ(
        frame1.value(),
        kFrame1
    );

    EXPECT_EQ(
        frame2.value(),
        kFrame2
    );

    EXPECT_NE(
        frame0.value(),
        frame1.value()
    );

    EXPECT_NE(
        frame0.value(),
        frame2.value()
    );

    EXPECT_NE(
        frame1.value(),
        frame2.value()
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        frame0
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        frame1
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        frame2
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        frame0
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        frame1
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage2),
        frame2
    );

    EXPECT_EQ(
        pageTable.size(),
        3U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        3U
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage0)
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage1)
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage2)
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Duplicate Page Mapping
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_DuplicatePageMappingFailsWithoutChangingState
)
{
    // Arrange
    const auto originalFrame =
        integration.mapPage(kPage0);

    ASSERT_TRUE(
        originalFrame.has_value()
    );

    const auto originalPageTableSize =
        pageTable.size();

    const auto originalAllocatedFrameCount =
        physicalMemory.allocatedFrameCount();

    // Act
    const auto duplicateFrame =
        integration.mapPage(kPage0);

    // Assert
    EXPECT_FALSE(
        duplicateFrame.has_value()
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        originalFrame
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        originalFrame
    );

    EXPECT_EQ(
        pageTable.size(),
        originalPageTableSize
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        originalAllocatedFrameCount
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage0)
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Physical Memory Allocation Failure
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_AllocationFailureDoesNotCreatePageTableMapping
)
{
    // Arrange
    ASSERT_TRUE(
        integration.mapPage(kPage0).has_value()
    );

    ASSERT_TRUE(
        integration.mapPage(kPage1).has_value()
    );

    ASSERT_TRUE(
        integration.mapPage(kPage2).has_value()
    );

    ASSERT_TRUE(
        integration.mapPage(kPage3).has_value()
    );

    ASSERT_EQ(
        physicalMemory.allocatedFrameCount(),
        4U
    );

    ASSERT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    const PageId unavailablePage{
        100
    };

    // Act
    const auto result =
        integration.mapPage(unavailablePage);

    // Assert
    EXPECT_FALSE(
        result.has_value()
    );

    EXPECT_FALSE(
        pageTable.isMapped(unavailablePage)
    );

    EXPECT_FALSE(
        physicalMemory.isPageMapped(unavailablePage)
    );

    EXPECT_EQ(
        pageTable.size(),
        4U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        4U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Unmapping
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_UnmapPageRemovesMappingAndReleasesFrame
)
{
    // Arrange
    const auto frameId =
        integration.mapPage(kPage0);

    ASSERT_TRUE(
        frameId.has_value()
    );

    ASSERT_TRUE(
        pageTable.isMapped(kPage0)
    );

    ASSERT_FALSE(
        physicalMemory.isFrameFree(frameId.value())
    );

    // Act
    const bool result =
        integration.unmapPage(kPage0);

    // Assert
    ASSERT_TRUE(
        result
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_FALSE(
        physicalMemory.isPageMapped(kPage0)
    );

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage0).has_value()
    );

    EXPECT_TRUE(
        physicalMemory.isFrameFree(frameId.value())
    );

    EXPECT_EQ(
        pageTable.size(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        4U
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Unmapping Unknown Page
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_UnmappingUnknownPageFailsWithoutChangingState
)
{
    // Arrange
    EXPECT_TRUE(
        pageTable.empty()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    // Act
    const bool result =
        integration.unmapPage(kPage0);

    // Assert
    EXPECT_FALSE(
        result
    );

    EXPECT_TRUE(
        pageTable.empty()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        4U
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Frame Release
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_ReleasingFrameRemovesCorrespondingPageTableMapping
)
{
    // Arrange
    const auto frameId =
        integration.mapPage(kPage0);

    ASSERT_TRUE(
        frameId.has_value()
    );

    ASSERT_EQ(
        pageTable.lookup(kPage0),
        frameId
    );

    ASSERT_EQ(
        physicalMemory.frameForPage(kPage0),
        frameId
    );

    // Act
    const bool result =
        integration.releaseFrame(frameId.value());

    // Assert
    ASSERT_TRUE(
        result
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_FALSE(
        physicalMemory.isPageMapped(kPage0)
    );

    EXPECT_FALSE(
        physicalMemory.frameForPage(kPage0).has_value()
    );

    EXPECT_TRUE(
        physicalMemory.isFrameFree(frameId.value())
    );

    EXPECT_EQ(
        pageTable.size(),
        0U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Releasing an Unallocated Frame
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_ReleasingFreeFrameFailsWithoutChangingState
)
{
    // Arrange
    EXPECT_TRUE(
        physicalMemory.isFrameFree(kFrame0)
    );

    EXPECT_TRUE(
        pageTable.empty()
    );

    // Act
    const bool result =
        integration.releaseFrame(kFrame0);

    // Assert
    EXPECT_FALSE(
        result
    );

    EXPECT_TRUE(
        physicalMemory.isFrameFree(kFrame0)
    );

    EXPECT_TRUE(
        pageTable.empty()
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        0U
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Frame Reuse
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_ReleasedFrameCanBeReusedByAnotherPage
)
{
    // Arrange
    const auto firstFrame =
        integration.mapPage(kPage0);

    ASSERT_TRUE(
        firstFrame.has_value()
    );

    ASSERT_EQ(
        firstFrame.value(),
        kFrame0
    );

    // Act
    ASSERT_TRUE(
        integration.unmapPage(kPage0)
    );

    const auto secondFrame =
        integration.mapPage(kPage1);

    // Assert
    ASSERT_TRUE(
        secondFrame.has_value()
    );

    EXPECT_EQ(
        secondFrame.value(),
        kFrame0
    );

    EXPECT_FALSE(
        pageTable.isMapped(kPage0)
    );

    EXPECT_FALSE(
        physicalMemory.isPageMapped(kPage0)
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        secondFrame
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        secondFrame
    );

    EXPECT_TRUE(
        integration.isMappingConsistent(kPage1)
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}


// ============================================================================
// US-203: Consistency Verification
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_PageTablePhysicalMemoryAndFrameRemainConsistent
)
{
    // Arrange
    const auto frameId =
        integration.mapPage(kPage0);

    ASSERT_TRUE(
        frameId.has_value()
    );

    // Act
    const bool mappingConsistent =
        integration.isMappingConsistent(kPage0);

    const bool systemConsistent =
        integration.isConsistent();

    // Assert
    EXPECT_TRUE(
        mappingConsistent
    );

    EXPECT_TRUE(
        systemConsistent
    );

    EXPECT_EQ(
        pageTable.lookup(kPage0),
        frameId
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        frameId
    );

    const Frame* frame =
        physicalMemory.frame(frameId.value());

    ASSERT_NE(
        frame,
        nullptr
    );

    ASSERT_TRUE(
        frame->mappedPage().has_value()
    );

    EXPECT_EQ(
        frame->mappedPage().value(),
        kPage0
    );
}


// ============================================================================
// US-203: Conflicting PageTable Mapping
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_ConflictingPageTableMappingIsRejected
)
{
    // Arrange
    const auto physicalFrame =
        physicalMemory.allocateFrame(kPage0);

    ASSERT_TRUE(
        physicalFrame.has_value()
    );

    /*
     * Deliberately create an inconsistent state:
     *
     * PhysicalMemoryManager:
     *
     *     Page0 -> Frame0
     *
     * PageTable:
     *
     *     Page1 -> Frame0
     */
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            physicalFrame.value()
        )
    );

    // Act
    const bool result =
        integration.isMappingConsistent(kPage1);

    // Assert
    EXPECT_FALSE(
        result
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        physicalFrame
    );

    EXPECT_FALSE(
        physicalMemory.isPageMapped(kPage1)
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        physicalFrame
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        1U
    );
}


// ============================================================================
// US-203: Conflicting Mapping Prevents Destructive Release
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_ConflictingMappingPreventsFrameRelease
)
{
    // Arrange
    const auto physicalFrame =
        physicalMemory.allocateFrame(kPage0);

    ASSERT_TRUE(
        physicalFrame.has_value()
    );

    /*
     * Deliberately create:
     *
     * PhysicalMemoryManager:
     *
     *     Page0 -> Frame0
     *
     * PageTable:
     *
     *     Page1 -> Frame0
     */
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            physicalFrame.value()
        )
    );

    // Act
    const bool result =
        integration.releaseFrame(physicalFrame.value());

    // Assert
    EXPECT_FALSE(
        result
    );

    /*
     * Frame0 must remain allocated to Page0.
     */
    EXPECT_FALSE(
        physicalMemory.isFrameFree(
            physicalFrame.value()
        )
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        physicalFrame
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        1U
    );

    /*
     * The conflicting PageTable entry remains untouched.
     */
    EXPECT_EQ(
        pageTable.lookup(kPage1),
        physicalFrame
    );
}


// ============================================================================
// US-203: Conflicting Unmap Does Not Release Wrong Frame
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_ConflictingUnmapDoesNotReleaseWrongFrame
)
{
    // Arrange
    const auto physicalFrame =
        physicalMemory.allocateFrame(kPage0);

    ASSERT_TRUE(
        physicalFrame.has_value()
    );

    /*
     * Deliberately create:
     *
     * PhysicalMemoryManager:
     *
     *     Page0 -> Frame0
     *
     * PageTable:
     *
     *     Page1 -> Frame0
     */
    ASSERT_TRUE(
        pageTable.map(
            kPage1,
            physicalFrame.value()
        )
    );

    // Act
    const bool result =
        integration.unmapPage(kPage1);

    // Assert
    EXPECT_FALSE(
        result
    );

    /*
     * Page0 still owns Frame0.
     */
    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        physicalFrame
    );

    EXPECT_FALSE(
        physicalMemory.isFrameFree(
            physicalFrame.value()
        )
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        1U
    );

    /*
     * The conflicting PageTable mapping was not removed because the
     * physical-memory relationship could not be validated safely.
     */
    EXPECT_EQ(
        pageTable.lookup(kPage1),
        physicalFrame
    );
}


// ============================================================================
// US-203: State Remains Consistent After Failed Mapping
// ============================================================================

TEST_F(
    PageTablePhysicalMemoryIntegrationTest,
    US203_StateRemainsConsistentAfterFailedMapping
)
{
    // Arrange
    ASSERT_TRUE(
        integration.mapPage(kPage0).has_value()
    );

    ASSERT_TRUE(
        integration.mapPage(kPage1).has_value()
    );

    ASSERT_TRUE(
        integration.mapPage(kPage2).has_value()
    );

    ASSERT_TRUE(
        integration.mapPage(kPage3).has_value()
    );

    const PageId failedPage{
        100
    };

    // Act
    const auto result =
        integration.mapPage(failedPage);

    // Assert
    EXPECT_FALSE(
        result.has_value()
    );

    /*
     * Failed page was never published to the PageTable.
     */
    EXPECT_FALSE(
        pageTable.isMapped(failedPage)
    );

    /*
     * Failed page was never allocated a physical frame.
     */
    EXPECT_FALSE(
        physicalMemory.isPageMapped(failedPage)
    );

    /*
     * Existing mappings remain unchanged.
     */
    EXPECT_EQ(
        pageTable.lookup(kPage0),
        kFrame0
    );

    EXPECT_EQ(
        pageTable.lookup(kPage1),
        kFrame1
    );

    EXPECT_EQ(
        pageTable.lookup(kPage2),
        kFrame2
    );

    EXPECT_EQ(
        pageTable.lookup(kPage3),
        kFrame3
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage0),
        kFrame0
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage1),
        kFrame1
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage2),
        kFrame2
    );

    EXPECT_EQ(
        physicalMemory.frameForPage(kPage3),
        kFrame3
    );

    EXPECT_EQ(
        pageTable.size(),
        4U
    );

    EXPECT_EQ(
        physicalMemory.allocatedFrameCount(),
        4U
    );

    EXPECT_EQ(
        physicalMemory.freeFrameCount(),
        0U
    );

    EXPECT_TRUE(
        integration.isConsistent()
    );
}

} // namespace emmus::test