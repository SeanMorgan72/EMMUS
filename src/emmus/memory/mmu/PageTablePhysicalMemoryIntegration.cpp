#include "emmus/memory/mmu/PageTablePhysicalMemoryIntegration.hpp"

namespace emmus::memory::mmu
{

PageTablePhysicalMemoryIntegration::
PageTablePhysicalMemoryIntegration(
    PageTable& pageTable,
    physical::PhysicalMemoryManager& physicalMemory
) noexcept
    : pageTable_(pageTable),
      physicalMemory_(physicalMemory)
{
}


std::optional<PageTablePhysicalMemoryIntegration::FrameId>
PageTablePhysicalMemoryIntegration::mapPage(
    PageId pageId
) noexcept
{
    /*
     * The page must not already exist in the page table.
     *
     * Checking this before physical allocation guarantees that a conflicting
     * virtual mapping does not consume a physical frame.
     */
    if (pageTable_.isMapped(pageId))
    {
        return std::nullopt;
    }


    /*
     * PhysicalMemoryManager independently prevents duplicate physical
     * allocation for a PageId.
     *
     * Keep the check explicit here so the integration contract is clear.
     */
    if (physicalMemory_.isPageMapped(pageId))
    {
        return std::nullopt;
    }


    /*
     * IMPORTANT:
     *
     * Physical allocation occurs before PageTable::map().
     *
     * This guarantees that a PageTable entry can never reference a frame
     * that was not successfully allocated.
     */
    const auto frameId =
        physicalMemory_.allocateFrame(pageId);


    /*
     * Physical allocation failure leaves the PageTable untouched.
     */
    if (!frameId.has_value())
    {
        return std::nullopt;
    }


    /*
     * Verify the physical allocation before publishing the mapping.
     */
    const physical::Frame* allocatedFrame =
        physicalMemory_.frame(frameId.value());


    if (allocatedFrame == nullptr ||
        !allocatedFrame->isOccupied() ||
        !allocatedFrame->mappedPage().has_value() ||
        allocatedFrame->mappedPage().value() != pageId)
    {
        /*
         * Do not publish a PageTable entry for an invalid physical state.
         *
         * Only release the frame when we can prove that the frame contains
         * the page allocated by this operation. This prevents accidentally
         * releasing a frame belonging to some other page if an internal
         * invariant has already been violated.
         */
        if (allocatedFrame != nullptr &&
            allocatedFrame->mappedPage().has_value() &&
            allocatedFrame->mappedPage().value() == pageId)
        {
            static_cast<void>(
                physicalMemory_.releaseFrame(frameId.value())
            );
        }

        return std::nullopt;
    }


    /*
     * Publish the virtual-to-physical mapping only after the physical
     * allocation has succeeded and been verified.
     */
    if (!pageTable_.map(pageId, frameId.value()))
    {
        /*
         * PageTable::map() failed.
         *
         * Roll back the physical allocation so that the operation cannot
         * leave a frame allocated without a corresponding page-table entry.
         */
        static_cast<void>(
            physicalMemory_.releaseFrame(frameId.value())
        );

        return std::nullopt;
    }


    /*
     * At this point:
     *
     * PageTable:
     *     pageId -> frameId
     *
     * PhysicalMemoryManager:
     *     pageId -> frameId
     *
     * Frame:
     *     frameId -> pageId
     */
    return frameId;
}


bool PageTablePhysicalMemoryIntegration::unmapPage(
    PageId pageId
) noexcept
{
    /*
     * Obtain the physical frame from the page table first.
     */
    const auto frameId =
        pageTable_.lookup(pageId);


    /*
     * There is no integrated mapping to remove.
     */
    if (!frameId.has_value())
    {
        return false;
    }


    /*
     * Never modify either component when their existing relationship
     * is inconsistent.
     */
    if (!isMappingConsistent(pageId))
    {
        return false;
    }


    /*
     * Remove the page-table entry first.
     *
     * If physical release unexpectedly fails, the page-table mapping can
     * safely be restored because the frame is still occupied.
     */
    if (!pageTable_.unmap(pageId))
    {
        return false;
    }


    /*
     * Release the exact frame previously associated with the page.
     */
    if (!physicalMemory_.releaseFrame(frameId.value()))
    {
        /*
         * Roll back the PageTable operation.
         *
         * With the existing PhysicalMemoryManager implementation, release
         * should succeed after the consistency check above. This rollback
         * protects the integration boundary if that implementation changes.
         */
        static_cast<void>(
            pageTable_.map(pageId, frameId.value())
        );

        return false;
    }


    return true;
}


bool PageTablePhysicalMemoryIntegration::releaseFrame(
    FrameId frameId
) noexcept
{
    /*
     * The frame must exist and be occupied.
     */
    const physical::Frame* selectedFrame =
        physicalMemory_.frame(frameId);


    if (selectedFrame == nullptr ||
        !selectedFrame->isOccupied())
    {
        return false;
    }


    /*
     * An occupied frame must contain a page.
     */
    const auto pageId =
        selectedFrame->mappedPage();


    if (!pageId.has_value())
    {
        return false;
    }


    /*
     * Verify that the PageTable identifies this exact frame for the same
     * page before modifying anything.
     */
    const auto mappedFrame =
        pageTable_.lookup(pageId.value());


    if (!mappedFrame.has_value() ||
        mappedFrame.value() != frameId)
    {
        /*
         * Reject stale/conflicting state.
         *
         * In particular, never release a physical frame if the page table
         * points somewhere else or contains no mapping.
         */
        return false;
    }


    /*
     * Remove the virtual mapping first.
     */
    if (!pageTable_.unmap(pageId.value()))
    {
        return false;
    }


    /*
     * Release the exact physical frame.
     */
    if (!physicalMemory_.releaseFrame(frameId))
    {
        /*
         * The frame remains occupied if releaseFrame() fails, so restoring
         * the page-table mapping is safe.
         */
        static_cast<void>(
            pageTable_.map(pageId.value(), frameId)
        );

        return false;
    }


    return true;
}


std::optional<PageTablePhysicalMemoryIntegration::FrameId>
PageTablePhysicalMemoryIntegration::frameForPage(
    PageId pageId
) const noexcept
{
    if (!isMappingConsistent(pageId))
    {
        return std::nullopt;
    }

    return pageTable_.lookup(pageId);
}


bool PageTablePhysicalMemoryIntegration::isMappingConsistent(
    PageId pageId
) const noexcept
{
    /*
     * Obtain both views of the relationship.
     */
    const auto pageTableFrame =
        pageTable_.lookup(pageId);

    const auto physicalFrame =
        physicalMemory_.frameForPage(pageId);


    /*
     * Both components must know about the page.
     */
    if (!pageTableFrame.has_value() ||
        !physicalFrame.has_value())
    {
        return false;
    }


    /*
     * Both components must identify the same physical frame.
     */
    if (pageTableFrame.value() != physicalFrame.value())
    {
        return false;
    }


    /*
     * The Frame itself must agree with both components.
     */
    const physical::Frame* frame =
        physicalMemory_.frame(pageTableFrame.value());


    if (frame == nullptr ||
        !frame->isOccupied())
    {
        return false;
    }


    const auto& mappedPage =
        frame->mappedPage();


    return mappedPage.has_value() &&
           mappedPage.value() == pageId;
}


bool PageTablePhysicalMemoryIntegration::isConsistent() const noexcept
{
    /*
     * Every allocated physical frame should correspond to exactly one
     * page-table mapping.
     *
     * Because every successful physical allocation represents one page,
     * the counts must agree before the detailed relationship check.
     */
    if (pageTable_.size() !=
        physicalMemory_.allocatedFrameCount())
    {
        return false;
    }


    /*
     * Walk every managed physical frame and verify occupied frames against
     * the PageTable.
     *
     * PageTable intentionally encapsulates its unordered_map, so verification
     * is performed through the public lookup interface.
     */
    for (
        physical::PhysicalMemoryManager::FrameCount index = 0;
        index < physicalMemory_.capacity();
        ++index
    )
    {
        const auto frameId =
            physical::PhysicalMemoryManager::FrameId{
                static_cast<
                    physical::PhysicalMemoryManager::FrameId::ValueType
                >(index)
            };


        const physical::Frame* frame =
            physicalMemory_.frame(frameId);


        /*
         * Invalid frame access should never occur because the FrameId was
         * generated from the manager's capacity.
         */
        if (frame == nullptr)
        {
            return false;
        }


        /*
         * Free frames do not require a page-table mapping.
         */
        if (!frame->isOccupied())
        {
            continue;
        }


        /*
         * An occupied frame must contain a PageId.
         */
        const auto pageId =
            frame->mappedPage();


        if (!pageId.has_value())
        {
            return false;
        }


        /*
         * The PageTable must identify this exact frame for the page.
         */
        if (!isMappingConsistent(pageId.value()))
        {
            return false;
        }
    }


    return true;
}

} // namespace emmus::memory::mmu